#include <iostream>
#include<vector>
#include<conio.h>
#include<string>
#include<chrono>
#include<thread>
#include<pthread.h>
#include<Windows.h>

#define NUM_OF_ROWS 20
#define NUM_OF_COLS 45
#define AI_MILLISECOND_SPEED 1000

using namespace std;

/* colors
http://www.cplusplus.com/forum/beginner/5830/
[edit] Oh, yeah, colors are bit-encoded thus:
bit 0 - foreground blue
bit 1 - foreground green
bit 2 - foreground red
bit 3 - foreground intensity
bit 4 - background blue
bit 5 - background green
bit 6 - background red
bit 7 - background intensity
*/

bool colorsEnabled = 0;
class Screen;
class Player;
class Bullet;

Screen* gameView;
enum Directions{
	UP = 2,
	DOWN = -2,
	LEFT = -1,
	RIGHT = 1
};

Directions getOppositeDirection(Directions dir){
	switch (dir){
	case UP:
		return DOWN;
		break;
	case DOWN:
		return UP;
		break;
	case LEFT:
		return RIGHT;
		break;
	case RIGHT:
		return LEFT;
		break;
	default:
		system("cls");
		cout << "error";
		system("pause");
		break;
	}
}

class Point{
public:
	int x, y;
};



class OnScreen{
public:
	OnScreen(){
		visibleItems.push_back(this);
	}
	virtual ~OnScreen(){
		for(vector<OnScreen*>::iterator iter = visibleItems.begin(); iter != visibleItems.end(); iter++){
			OnScreen *item = *iter;
			if(item == this){
				visibleItems.erase(iter);
				break;
			}
		}
	}
	bool isInScreenBounds(){
		return (this->location.x < NUM_OF_COLS && this->location.y < NUM_OF_ROWS && this->location.x >=0 && this->location.y >= 0);
	}
	static vector<OnScreen*> visibleItems;
	virtual char toChar()=0;
	Point getLocation(){
		return location;
	}
	bool intersects(OnScreen *other){
		return (this->location.x == other->location.x && this->location.y == other->location.y);
	}
protected:
	virtual bool move(Directions dir, int speed){ // return 0 if couldn't move safely
		switch(dir){
		case Directions::UP:
			location.y-=speed;
			break;
		case Directions::DOWN:
			location.y+=speed;
			break;
		case Directions::LEFT:
			location.x-=speed;
			break;
		case Directions::RIGHT:
			location.x+=speed;
			break;
		}
		if(!isInScreenBounds()){
			OnScreen::move(getOppositeDirection(dir), speed);
			return false;
		}
		return true;
	}
	Point location;

};

vector<OnScreen*> OnScreen::visibleItems;





class LivingEntity : public OnScreen{
public:
	LivingEntity(){
		livingThings.push_back(this);
	}
	virtual ~LivingEntity(){
		for(vector<LivingEntity*>::iterator iter = livingThings.begin(); iter != livingThings.end(); iter++){
			LivingEntity *item = *iter;
			if(item == this){
				livingThings.erase(iter);
				break;
			}
		}
	}
	virtual void subHealth(int amount){
		health-=amount;
		if(!isAlive()){
			delete this;
		}
	}
	bool isAlive(){
		return (health>0);
	};
	Directions facing;
	static vector<LivingEntity*> livingThings;
protected:
	float health;
	int speed;
};

vector<LivingEntity*> LivingEntity::livingThings;
class Bullet : public OnScreen{
public:
	Bullet(int sparg, Directions dir, int x, int y, int power = 10){
		this->speed = sparg;
		this->direction = dir;
		this->location.x = x;
		this->location.y = y;
		this->power = power;
		Bullet::addBullet(this);
	}
	~Bullet(){
		for(vector<Bullet*>::iterator iter = bullets.begin(); iter != bullets.end(); iter++){
			OnScreen *item = *iter;
			if(item == this){
				bullets.erase(iter);
				break;
			}
		}
	}

	static void renderBullets(){
		for(unsigned int i = 0; i < bullets.size(); i++){
			Bullet* bullet = bullets[i];
			bool sucess = bullet->move(); // bullet cannot move if false
			if(!sucess){
				delete bullet;
			}
			else{
				for(vector<LivingEntity*>::iterator iter = LivingEntity::livingThings.begin(); iter != LivingEntity::livingThings.end(); iter++){
					LivingEntity *item = *iter;
					if(item->intersects(bullet)){ // player has hit bullet
						item->subHealth(bullet->power);
						delete bullet;
						break;
					}
				}
			}
		}
	}
protected:
	int speed, power;
	Directions direction;
	static vector<Bullet*> bullets;
	static void addBullet(Bullet* bullet){
		bullets.push_back(bullet);
	}
	virtual bool move(){
		return OnScreen::move(direction, speed);
	}
	virtual char toChar(){
		return '*';
	}
};

vector<Bullet*> Bullet::bullets;

class FightingEntity : public LivingEntity{
public:
	int ammo, magSize;
	void makeShootSound(){
		pthread_t soundThread;
		if(!pthread_create(&soundThread, NULL, &FightingEntity::actuallyBeep, this));
	}

	virtual void shoot(Directions){
		makeShootSound();
	}
private:
	static void* actuallyBeep(void* context){
		Beep(523, 500);
		return NULL;
	}
};

class Player : public FightingEntity{
	friend class Screen;

public:
	Player(int row, int col, float health, int ammo, int score, int speed, string name = "Player 1", Directions facing = Directions::RIGHT){
		this->location.x = col;
		this->location.y = row;
		this->health = health;
		this->score = score;
		this->ammo = ammo;
		this->speed = speed;
		this->name = name;
		this->facing = facing;
		this->magSize = ammo;
	}
	string getName(){
		return name;
	}
	void move(Directions dir){
		facing = dir;
		OnScreen::move(facing, speed);
	}
	void shoot(Directions dir){
		FightingEntity::shoot(dir);
		if(this->ammo > 0){
			Bullet *shot = new Bullet(1, facing, this->location.x, this->location.y);
			ammo--;
		}
		else{
			ammo = magSize;
		}
	}
	void subHealth(int amount);
protected:
	string name;
	int score;

	virtual char toChar(){
		return 'P';
	}
};

class Enemy : public FightingEntity{
public:
	Enemy(){
		enemies.push_back(this);
	}
	~Enemy(){
		for(vector<Enemy*>::iterator iter = enemies.begin(); iter != enemies.end(); iter++){
			Enemy *item = *iter;
			if(item == this){
				enemies.erase(iter);
				break;
			}
		}
	}
	static void aiTurn(LivingEntity *target){
		for(vector<Enemy*>::iterator iter = enemies.begin(); iter != enemies.end(); iter++){
			Enemy *item = *iter;
			item->processAndAct(target);
		}
	}
	static vector<Enemy*> getEnemies(){
		vector<Enemy*> retval = enemies;
		return retval;
	}
	virtual void processAndAct(LivingEntity *target)=0;
protected:
	static vector<Enemy*> enemies;
};

vector<Enemy*> Enemy::enemies;

class EnemySoldier : public Enemy{
public:
	EnemySoldier(int row, int col, float health = 100.0, int ammo = 1, int magsize = 1, int speed = 1){
		this->location.x = col;
		this->location.y = row;
		this->health = health;
		this->ammo = ammo;
		this->speed = speed;
		this->magSize = magSize;
	}
	void shoot(Directions dir){
		FightingEntity::shoot(dir);
		Bullet *shot = new Bullet(1, facing, this->location.x, this->location.y);
		ammo--;
	}
	void processAndAct(LivingEntity *target){
		Point* targetLocation = &target->getLocation();
		if(targetLocation->x != this->location.x){
			if(targetLocation->y > this->location.y){ // move
				this->move(Directions::DOWN, this->speed);
			}
			else if(targetLocation->y < this->location.y){ // move
				this->move(Directions::UP, this->speed);
			}
			else if(ammo == 0){ // no ammo
				ammo = magSize;
			}
			else{ // turn and shoot
				if(targetLocation->x > this->location.x){
					this->facing = Directions::RIGHT;
				}
				else{
					this->facing = Directions::LEFT;
				}
				this->shoot(this->facing);
			}
		}
		else if(ammo == 0){
			ammo = magSize;
		}
		else{ // turn and shoot
			if(targetLocation->y > this->location.y){
				this->facing = Directions::DOWN;
			}
			else{
				this->facing = Directions::UP;
			}
			this->shoot(this->facing);
		}
	}
protected:
	virtual char toChar(){
		return 'E';
	}
};

class Screen {

public:
	Screen(char initial, Player *mainPlayer){
		this->color = 0;
		this->colorChangedFlag = false;
		this->blank = initial;
		this->mainPlayer = mainPlayer;
		memset(&screen[0][0], initial, sizeof(screen)); // in lieu of double for
	}
	bool stuffAt(int r, int c){
		if(screen[r][c] == '-'){ 
			return false;
		}
		return true;
	}
	void render(){
		system("cls");
		if(colorsEnabled){
			if(colorChangedFlag){
				if(color){
					char lColor[] = "Color @@";
					// split hex number doesn't work correctly
					int secondNum = color%0x10; // maybe need to change it to 0x10 (0xF is like 9 for dec)
					color/= 0x10;
					int firstNum = color;

					if(firstNum <= 9){
						lColor[6] = firstNum+48; // add ASCII offset
					}
					else{
						lColor[6] = firstNum+55; // add ascii offset (A+55 = 65, character code for 'A')
					}

					if(secondNum <= 9){
						lColor[7] = secondNum+48; // add ASCII offset
					}
					else{
						lColor[7] = secondNum+55; // add ascii odffset (A+55 = 65, character code for 'A')
					}
				
					system(lColor);
					color = 0;
				}
				else{
					system("Color 07"); // bit 0, 1, 2
					colorChangedFlag = false;
				}
			}
		}

		memset(&screen[0][0], this->blank, sizeof(screen)); // reset screen
		cout << mainPlayer->getName() << ":  Ammo:  " << mainPlayer->ammo << " Health: " << mainPlayer->health << endl; // playerinfo
		Bullet::renderBullets();
		for(vector<OnScreen*>::iterator iter = OnScreen::visibleItems.begin(); iter != OnScreen::visibleItems.end(); iter++){
			OnScreen *item = *iter;
			screen[item->getLocation().y][item->getLocation().x] = item->toChar();
		}

		for(int r = 0; r < NUM_OF_ROWS; r++){ // render screen
			for(int c = 0; c < NUM_OF_COLS; c++){
				cout << this->screen[r][c];
			}
			cout << "\n";
		}	
	}
	// color getters/setters
	void setColor(unsigned char color){
		this->color = color;
		colorChangedFlag = true;
	}
protected:
	char screen[NUM_OF_ROWS][NUM_OF_COLS];
	char blank;
	Player *mainPlayer;
	unsigned char color;
private:
	bool colorChangedFlag;
};

// things that cannot use forward declarations
void Player::subHealth(int amount){
	gameView->setColor(0x84); //1000 0100, or foreground red, background intense
	FightingEntity::subHealth(amount);
}

void getInput(Player* player){
	if(kbhit()){
		char input = getch();
		switch (input){
		case 'w':
		case 'W':
			player->move(Directions::UP);
			break;
		case 's':
		case 'S':
			player->move(Directions::DOWN);
			break;
		case 'a':
		case 'A':
			player->move(Directions::LEFT);
			break;
		case 'd':
		case 'D':
			player->move(Directions::RIGHT);
			break;
		case ' ':
			player->shoot(player->facing);
			break;
		default:
			break;
		}
	}
}

pthread_mutex_t ai_player_mutex = PTHREAD_MUTEX_INITIALIZER;

void* processaiwhole(void* args){ // multithread wrapper
	Player *p = reinterpret_cast<Player*>(args);
	while(true){
		pthread_mutex_lock(&ai_player_mutex);
		Enemy::aiTurn(p);
		pthread_mutex_unlock(&ai_player_mutex);
		this_thread::sleep_for(chrono::milliseconds(AI_MILLISECOND_SPEED));
	}
	return NULL;
}

void* processaibysingle(void* args){ // multithread wrapper
	Player *p = reinterpret_cast<Player*>(args);
	while(true){
		const int numberOfEnemies = Enemy::getEnemies().size(); // at beginning of thread
		for(int i = 0; !(pthread_mutex_lock(&ai_player_mutex)) && i < Enemy::getEnemies().size(); i++){ // if lock was sucessful
			Enemy* enemy = Enemy::getEnemies()[i];	
			enemy->processAndAct(p);
			pthread_mutex_unlock(&ai_player_mutex);
			this_thread::sleep_for(chrono::milliseconds(AI_MILLISECOND_SPEED/numberOfEnemies));
		}
		pthread_mutex_unlock(&ai_player_mutex); // needed to unlock the last check
	}
	return NULL;
}

void levelOne(Screen* screen){

	if(rand()%100 == 0){
		int randcoordinates = 0;
		do{
			randcoordinates = rand()%20+1;
		}while(screen->stuffAt(randcoordinates, 1));
		EnemySoldier *newsoldier = new EnemySoldier(randcoordinates, 1, 10);
	}
}

void mainMenu(){
	cout << "A little (glitchy) game that I made just for you. Hope you like it!" << endl << "Now, do you want (0) no colors and no lag or (1) colors and lag?" << endl << "Anything else, and it'll just be random" << endl;
	cin >> colorsEnabled;
	system("cls");
}

int main(){
	
	mainMenu();

	Player *player = new Player(5, 5, 100.0, 10, 0, 1);
	EnemySoldier *soldier = new EnemySoldier(8, 17, 10);
	EnemySoldier *soldier2 = new EnemySoldier(10, 5, 10);
	EnemySoldier *soldier3 = new EnemySoldier(20, 8, 10);
	EnemySoldier *soldier4 = new EnemySoldier(2, 2, 10);
	gameView = new Screen('-', player);

	pthread_t aiThread;
	/*cout << typeid(player).name() << endl;
	system("pause");*/
	//pthread_create(&aiThread, NULL, processaiwhole, player);
	pthread_create(&aiThread, NULL, processaibysingle, player);
	srand(time(NULL));
	while (player->isAlive() && Enemy::getEnemies().size()>0){
		pthread_mutex_lock(&ai_player_mutex);
		levelOne(gameView);
		getInput(player);
		gameView->render();
		pthread_mutex_unlock(&ai_player_mutex);
		this_thread::sleep_for(chrono::milliseconds(1)); // just enough time to allow for processai
	}
	system("cls");
	system("Color 07");
	if(player->isAlive()){
		cout << "you win!";
	}
	else{
		cout << "you lose!";
	}
	cout << endl;
	system("pause");
	//pthread_join(aiThread, NULL); don't do, AI never stops
	return 0;
}