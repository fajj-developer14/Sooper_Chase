#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <cmath>
#include <random>
#include <algorithm>

void setRawMode(bool enable) {
    static struct termios oldt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        struct termios newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // Disable buffering and local echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings
    }
}

bool kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

enum class Direction {
   None,
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
      int x, y;
      gameGrid& map;
      std::vector<Direction> directionField;

   
   public:
   Player(gameGrid& m): map(m), directionField(map.getHeight()*map.getWidth(), Direction::None) {
      x = y = 1;
      map.setTile(x, y, TileType::PLAYER);
   }

   void move(char letter) {
      map.setTile(x, y, TileType::SPACE);
      switch (letter)
      {
      case 'w':
         map.getTile(x, y-1) != TileType::WALL ? --y : 0;
         break;
      case 's':
         map.getTile(x, y+1) != TileType::WALL ? ++y : 0;
         break;
      case 'a':
         map.getTile(x-1, y) != TileType::WALL ? x-=1 : 0;
         break;
      case 'd':
         map.getTile(x+1, y) != TileType::WALL ? x+=1 : 0;
         break;
      default:
         break;
      }
      map.setTile(x, y, TileType::PLAYER);
   }
   void setDirectionField() {
      std::vector<bool> visited(map.getHeight()*map.getWidth(), false);
      std::queue<int> q;
      int possibleMoves[] = {-1, 1, 0, 0 };


      Direction directionLabel[] = {Direction::RIGHT, Direction::LEFT, Direction::UP, Direction::DOWN};
      
      q.push(map.toIndex(x, y));
      visited[map.toIndex(x,y)] = true;
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
      std::cout << std::endl;
   }

   std::vector<Direction> getDirectionField() {
      return directionField;
   }
};

class Enemy {
   private:
      gameGrid& map;
      Player& player;
      int x, y;
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
      static int accumulatedFrames = 0;
      int framesPerStep = 10;
      ++accumulatedFrames;
      // std::cout << "x: " << x << "y: " << y << "          " <<  std::endl;
      // std::cout << map.toIndex(x, y) << "          " << std::endl;
      // std::cout << directionField[map.toIndex(x, y)] << "          " << std::endl;
      while (accumulatedFrames >= framesPerStep) {
         Direction direction = directionField[map.toIndex(x, y)];
         // std::cout << static_cast<int>(direction) << std::endl;
         map.setTile(x,y, TileType::SPACE);
         switch (direction) {
            case Direction::UP:
               --y;
               break;
            case Direction::DOWN:
               ++y;
               break;
            case Direction::LEFT:
               x-=1;
               break;
            case Direction::RIGHT:
               x+=1;
               break;
         }
         map.setTile(x,y, TileType::ENEMY);
         accumulatedFrames -= framesPerStep;
      }
   }
};

int main() {
   setRawMode(true);
   srand(time(0));
   std::cout << "\033[?25l" << std::flush;
   bool isRunning = true;
   gameGrid worldMap(18, 80);
   Player player(worldMap);
   Enemy enemy(worldMap, player);


   char letter;

   while (isRunning) {
      std::cout << "\033[H";
      letter = 0;

      if (kbhit()) letter = getchar();
      if (letter == 'q') isRunning = false;
      player.move(letter);
      player.setDirectionField();
      enemy.activate();
      
      for(int y=0; y < worldMap.getHeight(); ++y) {
         for (int x=0; x < worldMap.getWidth(); ++x) {
            TileType tile = worldMap.getTile(x, y);
            switch (tile)
            {
            case TileType::WALL:
               std::cout << "#";
               break;
            case TileType::SPACE:
               std::cout << " ";
               break;            
            case TileType::PLAYER:
               std::cout << "P";
               break;
            case TileType::ENEMY:
               std::cout << "X";
               break;
            default:
               break;
            }
         }
         std::cout << std::endl;
      }

      usleep(16000);
   }
   std::cout << "\033[?25h" << std::endl;
   setRawMode(false);
   return 0;
}
