# musickey
a class for midi  controller‘s key , 为midi控制器单个按键封装的一个类，使用这个类可以很容易写出midi控制器的核心代码
+ 这个代码需要和arduino mega2560开发板配合使用,其它开发板可参照修改对应串口使用方法即可

### 音乐按键类
#### 设计思路
1. 使用者只需要在按键的构造函数中定义某按键所在引脚、传送MIDI信息所用的串口地址、按键所对应的MIDI命令、音高、力度、触发方式、按键去抖延时时长、音符播放延时时长就可以实现按键自动播放。
2. 播放逻辑是，如果按住按键不放，会一直播放，放开按键后会继续播放设定的延时时长再停止播放（每个按键可设定不同的延时时长）
3. 对于钢琴、吉他等打击类乐器，一直按住按键，其实只是播放音源设定的最长延时时长，不会一直咚咚咚地响下去。
4. 以上处理方法是我理解的符合乐器原理的处理方式。
5. 上面这些逻辑都已经在音乐按键类内部完成处理。使用者只需要了解这个原理就可以了，使用方法见例程，只有几行代码。

#### 使用方法，超很简单，就二步。
1. 定义一个按键类，定义的时候给出这个按键所在：引脚pin、MIDI信号所使用的串口号地址、MIDI命令、音符音高、力度、触发电平(HIGH or LOW)、按键去抖延时时长、音符播放停止后还需延时的时长。
2. 在loop循环中调用autoPlay()方法就可以了。


#### 使用例子
```
#include <Arduino.h>
#include "delayer.h"
#include "musickey.h"



#define KEYNUMS 2  //按键数量
struct MusicKeys{
  uint8_t keyNums=KEYNUMS;  //有多少按键，按键数量 
  MusicKey *keys[KEYNUMS];  //装所有按键的数组
};

HardwareSerial *s1=&Serial1;  //取MIDI播放串口的地址。
MusicKeys myKeys={};

void initKey(){
    //下面开始定义每个音乐按键的功能，参数依次含义为:
    //引脚pin、MIDI信号所使用的串口号地址、MIDI命令、音符音高、力度、触发电平(HIGH or LOW)、按键去抖延时时长、音符播放停止后还需延时的时长。
    myKeys.keys[0]=new MusicKey(2,s1,0x90,0x30,120,0x00,0x00,5000);  
    myKeys.keys[1]=new MusicKey(3,s1,0x90,0x34,120,0x01,0x00,5000);  

}



void setup() {
  // put your setup code here, to run once:
  initKey();
  //Serial.begin(9600);
  
}

void loop() {
  // put your main code here, to run repeatedly:
//依次扫描按键，并进行播放，如果扫描中发现有按键按下就会自动播放
//由于扫描及播放处理的速度很快，同时按下多个键也可以“同时”播放
//由于midi传输速率是312500bps,这个“同时”其实有延时
//播放一个按键的时长约为1/(31250/(3*8))=接近一毫秒左右
//如果同时按下十个按键，则需要约10毫秒才能完成播放命令发送。
  for(int i=0;i<KEYNUMS;i++){
  
    myKeys.keys[i]->autoPlay();
    //myKeys.keys[i]->stopMIDI();
  }

}
```
