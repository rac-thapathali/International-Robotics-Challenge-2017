#include "Grid.h"
#include "Bot.h"
#include "Game.h"


Game game;// end of Game class;

void setup(){  
  Serial.begin(9600);
  while(!Serial){
  }

  pinMode(10, INPUT);               //Sensor
  pinMode(11, INPUT);               //Sensor
  pinMode(12, INPUT);               //Sensor
 
  pinMode(13, OUTPUT);              //Buzzer
  pinMode(3, OUTPUT);               //Left PWM
  pinMode(9, OUTPUT);               //Right PWM
  
  pinMode(5, OUTPUT);               //Motor
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  //clearEEPROM(); exit(0);

  EEPROM_readAnything(0, game);http://downloads.arduino.cc/packages/package_index.json file signature verification failed. File ignored.

  
  delay(1000);
  if (game.mode != WET){
    Serial.println("DrI");     //I am initializing dry run
    game.initializeDryRun();
    Serial.println("CDrI");   //Completed Initialization of Dry Run
  }
}

void loop(){
  Bot bot;
  
  float RPM = 150, rightRPM, leftRPM;
  int error = 0, previousError = 0, difference = 0;
  int Kp = 16, Kd = 0;
  
  if (game.mode != WET)
  {    //Dry RUN
    Serial.println("IDr");  //In Dry RUN
    int prevXOrient, prevYOrient;
    while(1){
      previousError = error;
      error = bot.getError();
      difference = previousError - error;
      rightRPM = (RPM + Kp * error - Kd * difference); leftRPM = (RPM - Kp * error + Kd*difference);
      if (error > 0) leftRPM /= 2;
      if (error < 0) rightRPM /= 2;
      bot.moveForward(leftRPM, rightRPM);
      if(bot.nodeDetect()>PATH){
        bot.beep();
        game.lastVertex.x += game.xOrient;
        game.lastVertex.y += game.yOrient;
        //Serial.println("\n\nLastVertex:");
        //Serial.println(game.lastVertex.x);
        //Serial.println(game.lastVertex.y);
        
        if (game.lastVertex.x == 7 && game.lastVertex.y == 3){
          //Serial.println("Saved Wet");
          game.simulateDryCompletion();
          game.mode = WET;
          //EEPROM_writeAnything(0, game);
          bot.stopMoving();
          bot.ReadyForWet();
          exit(0);
        }
      }
      
      if(Game::dryPath[game.completedSegments+1] == Vertex::getIndex(game.lastVertex.x, game.lastVertex.y)){
        
        Serial.println("ARV");  //Arrived
        game.completedSegments++;
        prevXOrient = game.xOrient;
        prevYOrient = game.yOrient;
        game.xOrient = Vertex::dx(Game::dryPath[game.completedSegments + 1],Game::dryPath[game.completedSegments]);  
        game.yOrient = Vertex::dy(Game::dryPath[game.completedSegments + 1],Game::dryPath[game.completedSegments]);
        if(game.xOrient != prevXOrient || game.yOrient != prevYOrient){
          if (game.xOrient == -1 * prevYOrient && game.yOrient == 1 * prevXOrient){
            bot.moveUntil(VERTEX);
            bot.moveLeft();
          }else if(game.xOrient == 1 * prevYOrient && game.yOrient == -1 * prevXOrient){
            bot.moveUntil(VERTEX);
            bot.moveRight();
          }
        }
      }
    }
  }
  else
    {   //game.mode = WET
      Serial.println("WR");   //I am in Wet Run --> Actual Run
      bot.initializeWetRun();
      Color color;
      int blockCount = 0, block1, block2, d1,d2, tempBlock, pathLength;
      int from, to;
      //#1 Find Location of Two Blocks
      for (int i=0; i<50; i++){
        if (game.vertex[i].type == BLOCK){
          blockCount++;
          if (blockCount == 1) block1 = i;  
          if (blockCount == 2) block2 = i;
        }
      }
      Serial.println(block1);
      Serial.println(block2);
      //#2 Find closer Block
      pathLength = game.findShortest(0,block1);
      if (game.findShortest(0,block1) > game.findShortest(0,block2)){
        tempBlock = block1;
        block1 = block2;
        block2 = tempBlock;
      }
      pathLength = game.findShortest(0,block1); 
      //#3 Find if The Block is at starting vertex
      if (pathLength == 0)
      {   //Just In Case Blockbase dappears at (0,0)
        Serial.println("Pathlength0");
        color = bot.readBlockColor();
        bot.gripBlock();
        game.vertex[Vertex::getIndex(0,0)].type = BLOCKBASE;
        bot.moveUntil(BLOCKBASE);
        bot.lastVertex.x = 0;
        bot.lastVertex.y = 0;
      }
      else
      {    //if pathLength != 0  //#4 When the Block is not at 0,0
        Serial.println("-1~0"); //I have to go (0, 0) from (-1,0)
        bot.moveUntil(VERTEX);
        bot.lastVertex.x = 0;
        bot.lastVertex.y = 0;
        //#5 Move the bot from origin to first block
        bot.traverse(game.wetPath, pathLength, game.vertex[block1].type);   //Bot traverses from (0,0) to block1 //This should also orient the bot in the direction of the destination
        
        Serial.println("LVAT"); //Last Vertex After Traversing
        Serial.println(bot.lastVertex.x);
        Serial.println(bot.lastVertex.y);
        Serial.println("LOrB"); //Last Orient Of Bot
        Serial.println(bot.xOrient);
        Serial.println(bot.yOrient);
        //#6 Find the color of the block  
        color = bot.readBlockColor();
        if (color == BLUE)
        {     //Find The target of for the first block
          if (game.arrow == NORTH){
            to = Vertex::getIndex(0, 4);
          }else{
            to = Vertex::getIndex(0, 1);
          }
        }
        else
        {  // color == RED
          to = Vertex::getIndex(1,4);
        }
        //TODO  : It might be necessary to make the VertexType of vertex[block1] to BLOCKBASE (currently BLOCK) otherwise the bot maybot travel from that point
        d1 = game.findShortest(block1, to);
        d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);
        //#7 Find if it is possible to carry and place the first block
        if (d1 == INF && d2 == INF){// not possible to carry first block, so move set target to another block
          tempBlock = block1;
          block1 = block2;
          block2 = block1;
          Serial.println("NxtB");   // I am moving towards next Block
          bot.traverse(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), block1, game.vertex[block1].type);
          color = bot.readBlockColor();
          if (color == BLUE){
            if (game.arrow == NORTH){
              to = Vertex::getIndex(0, 4);
            }else{
              to = Vertex::getIndex(0, 1);
            }
          }
          else
          {  // color == RED
            to = Vertex::getIndex(1,4);
          }
        }// Now it is 100% possible to move the block1        
        bot.gripBlock();  //First Block is Gripped
        game.vertex[block1].type = BLOCKBASE;
        Serial.println("B2BBC"); //Vertex Type Converted from BLOCK to BLOCKBASE
        d1 = game.findShortest(block1, to); d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);   //At least one is not infinite
        if (d1<d2){                   //It's shorter to move first block via BlockBase
          Serial.println("ViaBB");    //It's shorter to go via BlockBase
          bot.moveUntil(BLOCKBASE);
          bot.lastVertex.x = Vertex::getX(block1);
          bot.lastVertex.y = Vertex::getY(block1);
          d1 = game.findShortest(block1, to);
          Serial.println(d1);
          //Serial.println("Bye");delay(1000);exit(0);
          bot.traverse(game.wetPath, d1, game.vertex[to].type);
        }
        else
        {                                  //It's shorter to move first block via Last Vertex
          Serial.println("ViaLV\nRot-180");     //It's shorter to go via Last Vertex
          bot.rotate180();
          bot.moveUntil(VERTEX);
          d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);
          bot.traverse(game.wetPath, d2, game.vertex[to].type);
        }
        
        if (game.vertex[to].type == REDPIT){
          bot.fillPit();
          Serial.println("B1RF"); //Block1 Red and Filled in REDPIT    
        }
        else //game.vertex[to].type == TZ
        { 
          bot.fillTransferZone(); //go Tannai agadi and put slowly
          Serial.println("B1BT"); //Block1 Blue and Transferred to Transfer Zone
        }
       
      } //if pathLength != 0 

      Serial.println("LV:");
      Serial.println(bot.lastVertex.x);
      Serial.println(bot.lastVertex.y);
      
      if(game.vertex[to].type == TRANSFERZONE){     //It means block1 was blue
        bot.rotate180();
        //Adjust back to last vertex
        bot.moveUntil(game.vertex[to].type);
        
        to = block2;
        d1 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);
        bot.traverse(game.wetPath, d1, game.vertex[to].type);
        bot.gripBlock();  //Second Block is Gripped
        to = Vertex::getIndex(1, 4);                //If previous block was blue, then this block must be red
        d1 = game.findShortest(block2, to); d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);   //At least one is not infinite
        if (d1<d2){
          Serial.println("ViaBB");    //It's shorter to go via BlockBase
          Serial.println("Adj2BB"); //I have adjusteed to block base
          bot.moveUntil(BLOCKBASE);
          bot.lastVertex.x = Vertex::getX(block2);  bot.lastVertex.y = Vertex::getY(block2);
          d1 = game.findShortest(block2, to);
          bot.traverse(game.wetPath, d1, game.vertex[to].type);
        }else{
          Serial.println("ViaLV\nRot-180");    //It's shorter to go via Last Vertex
          bot.rotate180();
          Serial.println("Adj2LV"); //I have adjusteed to block base
          bot.moveUntil(game.vertex[Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y)].type);
          d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);
          bot.traverse(game.wetPath, d2, game.vertex[to].type);
        }
        bot.fillPit();
        Serial.println("PitFilled");
        bot.moveUntil(VERTEX);  // bot.moveUntil(DEPOSITED) is equivalent to and hence is replaced by bot.moveUntil(VERTEX)
      }else{//game.vertex[type] == REDPIT
        game.vertex[to].type = DEPOSITED;
        d1 = game.findShortest(to, block2);
        d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), block2);
        Serial.println(block1); Serial.println(block2); delay(2000); exit(0);
        if (d1 < d2){
          bot.moveUntil(VERTEX);  // bot.moveUntil(DEPOSITED) is equivalent to and hence is replaced by bot.moveUntil(VERTEX)
          bot.lastVertex.x = Vertex::getX(to);
          bot.lastVertex.y = Vertex::getY(to);
        }else{
          bot.rotate180();
          bot.moveUntil(game.vertex[Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y)].type); 
        }
        
        d1 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), block2);
        bot.traverse(game.wetPath, d1, BLOCK);
        if (game.arrow == NORTH){
          to = Vertex::getIndex(0, 4);
        }else{
          to = Vertex::getIndex(0, 1);
        }
        bot.gripBlock();
        game.vertex[block2].type = BLOCKBASE;
        Serial.println("B2BBC"); //Vertex Type Converted from BLOCK to BLOCKBASE
        d1 = game.findShortest(block2, to); d2 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);   //At least one is not infinite
        if (d1<d2){                   //It's shorter to move first block via BlockBase
          Serial.println("ViaBB");    //It's shorter to go via BlockBase
          bot.moveUntil(BLOCKBASE);
          bot.lastVertex.x = Vertex::getX(block2);
          bot.lastVertex.y = Vertex::getY(block2);
          d1 = game.findShortest(block2, to);
          bot.traverse(game.wetPath, d1, game.vertex[to].type);
        }
        else
        {                                  //It's shorter to move first block via Last Vertex
          Serial.println("ViaLV\nRot-180");     //It's shorter to go via Last Vertex
          bot.rotate180();
          bot.moveUntil(game.vertex[to].type);
          d1 = game.findShortest(Vertex::getIndex(bot.lastVertex.x, bot.lastVertex.y), to);
          bot.traverse(game.wetPath, d1, game.vertex[to].type);
        }
        bot.fillTransferZone();
        Serial.println("B2BT");   //Block2 transferred to TZ
        bot.rotate180();
        //Adjust back to last vertex
        bot.moveUntil(game.vertex[to].type);
        d1 = game.findShortest(to, Vertex::getIndex(1,4));
        bot.traverse(game.wetPath, d1, DEPOSITED);    //bot.traverse(game.wetPath, d1, VERTEX);
      }
      Serial.println("LeftGridComplete");

      
      delay(2000);
      
      exit(0);

      
  }

  
  
}
