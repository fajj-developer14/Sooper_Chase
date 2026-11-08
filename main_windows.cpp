#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <random>
#include <algorithm>
#include <string>

enum class Direction {
   NONE,
   UP,
   DOWN,
   LEFT,
   RIGHT
};

enum class TileType {
   SPACE = 0,
   WALL,
   COIN,
   ENEMY,
   PLAYER = 10
};

class gameGrid {
   private:
   int width, height;
   std::vector<TileType> grid;

   public:
   int toIndex(int x, int y) {
      return (y*width + x);
   }

   int getX(int index) {return index % width;}
   int getY(int index) {return index / width;}

   gameGrid(int h, int w) : width(w), height(h), grid(w*h, TileType::SPACE) {
      for (int i=0; i < width; i++) {
         grid[i] = TileType::WALL;
         grid[toIndex(i, height-1)] = TileType::WALL;
      }
      for (int i=0; i < height; i++) {
         grid[toIndex(0, i)] = TileType::WALL;
         grid[toIndex(width-1, i)] = TileType::WALL;
      }
   }

   TileType getTile(int x, int y) {
      int index = toIndex(x, y);
      if (index >= 0 && index < width*height) {
         return grid[index];
      }
      return TileType::WALL;
   }
      TileType getTile(int index) {
      if (index >= 0 && index < width*height) {
         return grid[index];
      }
      return TileType::WALL;
   }
   void setTile(int x, int y, TileType tile) {
      int index = toIndex(x, y);
      if (index >= 0 && index < width*height) {
         grid[index] = tile;
      }
   }

   int getHeight() {return height;}
   int getWidth() {return width;}

};

class Player {
   private:
      int x, y, score = 0;
      gameGrid& map;
      std::vector<Direction> directionField;


   public:
   Player(gameGrid& m): map(m), directionField(map.getHeight()*map.getWidth(), Direction::NONE) {
      x = y = 1;
      map.setTile(x, y, TileType::PLAYER);
   }

   void move(char letter) {
      map.setTile(x, y, TileType::SPACE);
      switch (letter)
      {
      case 'w':
         if (map.getTile(x, y-1) != TileType::WALL) --y;
         break;
      case 's':
         if (map.getTile(x, y+1) != TileType::WALL) ++y;
         break;
      case 'a':
         if (map.getTile(x-1, y) != TileType::WALL) --x;
         break;
      case 'd':
         if (map.getTile(x+1, y) != TileType::WALL) ++x;
         break;
      default:
         break;
      }
      if (map.getTile(x, y) == TileType::ENEMY) return;
      if (map.getTile(x,y) == TileType::COIN) ++score;
      map.setTile(x, y, TileType::PLAYER);
   }

   bool isCollidingWithEnemy() {
      return map.getTile(x,y) == TileType::ENEMY;
   }

   void setDirectionField() {
      std::vector<bool> visited(map.getHeight()*map.getWidth(), false);
      std::queue<int> q;
      int possibleMoves[] = {-1, 1, 0, 0 };


      Direction directionLabel[] = {Direction::RIGHT, Direction::LEFT, Direction::UP, Direction::DOWN};

      q.push(map.toIndex(x, y));
      visited[map.toIndex(x,y)] = true;
      directionField[map.toIndex(x,y)] = Direction::NONE;

      std::vector<int> indices = {0, 1, 2, 3};
      std::random_device rd;
      std::mt19937 g(rd());

      while (!q.empty()) {
         int curr = q.front();
         q.pop();
         std::shuffle(indices.begin(), indices.end(), g); //shuffled indices to remove the L-shape issue in bfs
         
         for (int i = 0; i < 4; ++i) {
            int nextNode = curr + possibleMoves[indices[i]] + possibleMoves[3-indices[i]]*map.getWidth();

            if (nextNode >= 0 && nextNode < visited.size() && map.getTile(nextNode) != TileType::WALL && !visited[nextNode]) {
               visited[nextNode] = true;
               q.push(nextNode);
               directionField[nextNode] = directionLabel[indices[i]];
            }
         }
      }
   }

   std::vector<Direction> getDirectionField() {
      return directionField;
   }
   int getScore() {
      return score;
   }
   void updateScore(int num) {
      score += num;
   }
};

class Enemy {
   private:
      gameGrid& map;
      Player& player;
      int x, y, framesPerStep = 10, accumulatedFrames = 0;

   public:
      Enemy(gameGrid& g, Player& p) : map(g), player(p) {
         do {
           x = rand() % (map.getWidth());
           y = rand() % (map.getHeight());
         } while(map.getTile(x,y) != TileType::SPACE);
         map.setTile(x, y, TileType::ENEMY);
      }
   void activate() {
      std::vector<Direction> directionField = player.getDirectionField();
      ++accumulatedFrames;

      while (accumulatedFrames >= framesPerStep) {
         Direction direction = directionField[map.toIndex(x, y)];
         map.setTile(x,y, TileType::SPACE);
         switch (direction) {
            case Direction::UP:
               --y;
               break;
            case Direction::DOWN:
               ++y;
               break;
            case Direction::LEFT:
               --x;
               break;
            case Direction::RIGHT:
               ++x;
               break;
            case Direction::NONE:
               break;
            default:
               break;
         }
         if (map.getTile(x, y) == TileType::COIN) player.updateScore(-1); //decrement score if enemy gets the coin
         map.setTile(x,y, TileType::ENEMY);
         accumulatedFrames -= framesPerStep;
      }
   }
};

 
void generateCoins(gameGrid& map, Player& player, int winningScore) {
   static int accumulatedFrames = 0;
   int x,y, framesPerCoin = 280;
   static bool lastCoin = false;
   ++accumulatedFrames;
   while (accumulatedFrames >= framesPerCoin) {
      do {
         x = rand() % (map.getWidth());
         y = rand() % (map.getHeight());
      } while(map.getTile(x,y) != TileType::SPACE);
      accumulatedFrames -= framesPerCoin;
      map.setTile(x, y, TileType::COIN);
   }
   lastCoin = player.getScore() == winningScore - 1;
}

void addWalls(gameGrid& map, Player& player) {
   static int accumulateFrames = 0, round = 1;
   int framesPerRound = 400;
   ++accumulateFrames;
   if (round == map.getHeight()-3) return;
   while(accumulateFrames >= framesPerRound) {
      for (int x =  1; x < map.getWidth() - 1; ++x) {
         if (map.getTile(x, round) == TileType::PLAYER) player.move('s');
         else if(map.getTile(x,round) == TileType::COIN) map.setTile(x, round+1, TileType::COIN);
         map.setTile(x, round, TileType::WALL);
      }
      accumulateFrames -= framesPerRound;
      ++round;
   }
}

int main() {
   srand(time(0));
   gameGrid worldMap(18, 50);
   Player player(worldMap);
   Enemy enemy(worldMap, player); 
   const int winningScore = 15;

   std::cout << "\033[?25l"; //to hide the cursor
   bool isRunning = true;    

   char letter;

   while (isRunning) {
      std::cout << "\033[H";   //to make the cursor appear at the top left in each loop

      if (_kbhit()) letter = _getch();
      // if (letter == 'q') isRunning = false;
      player.move(letter);
      player.setDirectionField();
      enemy.activate();
      generateCoins(worldMap, player, winningScore);
      addWalls(worldMap, player);
      isRunning = !player.isCollidingWithEnemy() && player.getScore() < winningScore;

      for(int y=0; y < worldMap.getHeight(); ++y) {
         for (int x=0; x < worldMap.getWidth(); ++x) {
            TileType tile = worldMap.getTile(x, y);
            switch (tile)
            {
            case TileType::WALL:
               std::cout << "🧱";
               break;
            case TileType::SPACE:
               std::cout << "  ";  //two spaces to cater for double width emojis
               break;
            case TileType::PLAYER:
               std::cout << "🐥";
               break;
            case TileType::ENEMY:
               std::cout << "🧟";
               break;
            case TileType::COIN:
               std::cout << (player.getScore() < winningScore - 1 ? "🪙 " : "🏁");  //trailing space for the double width and adding the last flag
               break;
            default:
               break;
            }
         }
         std::cout << std::endl;
      }
      std::cout << std::string(45, ' ') << "Score: " << player.getScore() << " " << std::endl;

      Sleep(16); //60fps
   }
   std::cout << "\033[?25h";  //to make the cursor reappear
   std::cout << std::string(45, ' ') << (player.getScore() != winningScore ? "GAME OVER" : "YOU WON") << std::endl;
   return 0;
}
