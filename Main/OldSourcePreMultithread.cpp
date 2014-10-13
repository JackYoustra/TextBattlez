#include <iostream>
#include<vector>
#include<conio.h>
#include<string>
#include<time.h>

#define NUM_OF_ROWS 20
#define NUM_OF_COLS 45

using namespace std;



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
	~OnScreen(){
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
	~LivingEntity(){
		for(vector<LivingEntity*>::iterator iter = livingThings.begin(); iter != livingThings.end(); iter++){
			LivingEntity *item = *iter;
			if(item == this){
				livingThings.erase(iter);
				break;
			}
		}
	}
	void subHealth(int amount){
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
	virtual void shoot(Directions) = 0;
};

class Player : public FightingEntity{
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
	}
	string getName(){
		return name;
	}
	void move(Directions dir){
		facing = dir;
		OnScreen::move(facing, speed);
	}
	void shoot(Directions dir){
		if(this->ammo > 0){
			Bullet *shot = new Bullet(1, facing, this->location.x, this->location.y);
			ammo--;
		}
	}
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
		Bullet *shot = new Bullet(1, facing, this->location.x, this->location.y);
		ammo--;
	}
	void processAndAct(LivingEntity *target){
		if(target->getLocation().y > this->location.y){ // move
			this->move(Directions::DOWN, this->speed);
		}
		else if(target->getLocation().y < this->location.y){ // move
			this->move(Directions::UP, this->speed);
		}
		else if(ammo == 0){ // no ammo
			ammo = magSize;
		}
		else{ // turn and shoot
			if(target->getLocation().x > this->location.x){
				this->facing = Directions::RIGHT;
			}
			else{
				this->facing = Directions::LEFT;
			}
			this->shoot(this->facing);
		}
	}
protected:
	virtual char toChar(){
		return 'E';
	}
};

class Screen{	
public:
	Screen(char initial, Player *mainPlayer){
		this->blank = initial;
		this->mainPlayer = mainPlayer;
		memset(&screen[0][0], initial, sizeof(screen)); // in lieu of double for
	}
	void render(){
		system("cls");
		memset(&screen[0][0], this->blank, sizeof(screen)); // reset screen
		cout << mainPlayer->getName() << ":  Ammo:  " << mainPlayer->ammo << endl;
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
protected:
	char screen[NUM_OF_ROWS][NUM_OF_COLS];
	char blank;
	Player *mainPlayer;
};

void getInput(Player* player){
	if(kbhit()){
		char input = getch();
		switch (input){
		case 'w':
			player->move(Directions::UP);
			break;
		case 's':
			player->move(Directions::DOWN);
			break;
		case 'a':
			player->move(Directions::LEFT);
			break;
		case 'd':
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

int main(){
	srand( time( NULL ) ); 
	Player *player = new Player(5, 5, 100.0, 100, 0, 1);
	EnemySoldier *soldier = new EnemySoldier(8, 17);
	Screen *gameView = new Screen('-', player);
	srand(time(NULL));
	while (player->isAlive()){
		Enemy::aiTurn(player); // to fix: timed threads? Or adaptive difficulty depending on player speed?
		getInput(player);
		gameView->render();
	}
	return 0;
}