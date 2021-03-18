#ifndef musickey_h
#define musickey_h
#include <Arduino.h>
#include "delayer.h"

class MusicKey
{
  private:
  
  int8_t Num;  //按键编号
  int8_t Pin;  //所在引脚
  int8_t triggerType=HIGH; //LOW 低电平触发，HIGH 高电平触发

  int8_t lastKeyStat;  //按键前一个时间的状态HIGH OR LOW
  int8_t keyStatB4Delay;  //按键开始延时去抖动时的状态。HIGH OR LOW


  int16_t playDelayTime=0; //如果playType=2,这个值表示固时长的值。
  int16_t shakeDelayTime=0;  //消除按键抖动的延j时间

  byte midiCmd=0x90;   //在midi的1号通道进行播放，0x91表示在midi的2号通道进行播放,一直到16号通道。
  byte midiNoteNum=45;  //音高
  byte midiVel=120;     //力度

  Delayer *shakeDelayer=NULL;  //去抖延时器
  Delayer *playDelayer=NULL;   //演奏延时器

  HardwareSerial *cmdSerial=NULL;  //用于发送MIDI命令的串口对象

private:  
 //保存PIN的当前值 
  void savePinValue();

  int readPin();
  
  //从上一次使用PIN后，PIN的状态是否改变过。
  bool pinIsChange();

  /*PIN是否从一个状态稳定地切换到了另一个状态
  pinIsChangeOk的逻辑，
  1. 读当前pin的值给keyStatB4Delay
  2. keyStatB4Delay值与lastKeyStat不同
  3. 延时时间到，读到的pin值 与keyStatB4Delay相同
  */
  bool pinIsChangeOk();

  void playMIDI();
  void stopMIDI();
  void delayStopMIDI();

public:

  MusicKey(int8_t pin,HardwareSerial *cmds, byte midi_cmd=0x90,byte midi_note_num=45,byte midi_vel=120,
  int8_t trigger_type=HIGH,int16_t shake_delay_time=0,int16_t play_delay_time=0,int8_t num=0);
  
   void autoPlay();
};

#endif



//==================================================================================================
//类定义的实现部分

/*参数说明:
pin:按键所在引脚
cmds:传送midi信号所使用的串口对象地址
midi_cmd:按键所对应的midi命令
midi_note_num:按键所对应的音高
mdid_vel: 按键所对应的力度
trigger_type:按键触发方式,LOW 低电平触发，HIGH 高电平触发
shake_delay_time:确认按键稳定按下需要的延时时长
play_delay_time:每次按键按下播放MIDI的时长
num:按键编号，方便户外互动调用的时候找按键用的，和实体按键上编号统一，基本用不到
*/
MusicKey::MusicKey(int8_t pin,HardwareSerial *cmds, byte midi_cmd,byte midi_note_num,byte midi_vel,
  int8_t trigger_type,int16_t shake_delay_time,int16_t play_delay_time,int8_t num){
    
    this->Pin=pin;
    this->cmdSerial=cmds;
    this->cmdSerial->begin(31250);
    
    this->midiCmd=midi_cmd;
    this->midiNoteNum=midi_note_num;
    this->midiVel=midi_vel;

    this->triggerType=trigger_type;
    
    this->shakeDelayTime=shake_delay_time;
    this->playDelayTime=play_delay_time;
    
    this->Num=num;

//生成两个延时器
    this->shakeDelayer= new Delayer(shake_delay_time);
    this->playDelayer = new Delayer(play_delay_time);
    this->playDelayer->setAllowStart(false);//最开始延音计时器是不被允许的，直到有音符弹奏了，才开始允许延音计时。
}

int MusicKey::readPin(){
    return digitalRead(this->Pin);
}

void MusicKey::savePinValue(){
    lastKeyStat= readPin();
}

//从上一次使用PIN后，PIN的状态是否改变过。
bool MusicKey::pinIsChange(){
    if(readPin()==lastKeyStat){
        return false;
    }
    return true;
}

bool MusicKey::pinIsChangeOk(){
//不管在延时到来之前lastKeyStat,被重新设置过几次，只要和延时到了j时候不一致，则不能算翻转成功。
//另外如果savePinValue()函数只被本函数调用，也不会出现lastKeyStat被随意更新的情况。

    //信号没有翻转过，直接返回。
    int pinValueTemp=readPin(); 
    
    //后面一个条件是j为了防止极端情况下后面的延时j检查代码执行不到导致延时j器无法归j零。
    if(pinValueTemp==lastKeyStat && shakeDelayer->isStarted()==false){
        return false;
    }

    //如果信号开始翻转，那么开始进行翻转检测延时的准备，延时准备工作只会执行一次
    //延时开始后就不会再被执行了。
    if(shakeDelayer->isStarted()==false){
        this->keyStatB4Delay=pinValueTemp;
    }

    //检查延时是否到
    if(shakeDelayer->isDelayed()){
        if(readPin()==keyStatB4Delay){
            
            //保存新的pin值状态
            //savePinValue();
            lastKeyStat=keyStatB4Delay;
            return true;
        }else{
            return false;
        }
    }
    return false;
}

void MusicKey::playMIDI(){
/*
  这个函数暂时只直接播放一次，等研究清楚MIDI的信号保持机制（就是按键一直按着，该发送什么MIDI信号）再完善。
  答：MIDI的信号保持机制就是，如果没有后来的信号，则不会消除上一个信号的影响
  */

    //要放音，先停掉前面正在播放的音符
    cmdSerial->write(midiCmd);
    cmdSerial->write(midiNoteNum);
    cmdSerial->write(0x00);
 
    //开始播放
    cmdSerial->write(midiCmd);
    cmdSerial->write(midiNoteNum);
    cmdSerial->write(midiVel);

    //playDelayer->setAllowStart(true);

}

//延时终止音符音符播放
void MusicKey::stopMIDI(){

    cmdSerial->write(midiCmd);
    cmdSerial->write(midiNoteNum);
    cmdSerial->write(0x00);

}

//延时终止音符音符播放
void MusicKey::delayStopMIDI(){
    if(playDelayer->isDelayed()){
        cmdSerial->write(midiCmd);
        cmdSerial->write(midiNoteNum);
        cmdSerial->write(0x00);
        playDelayer->setAllowStart(false);
    }
}
//给用户使用的主要函数，自动扫描按键，自动播放MIDI信号。
void MusicKey::autoPlay(){

    if(pinIsChangeOk() ){
        //如果现在的电平信号和播放触发信号一致则播放
        if(lastKeyStat==triggerType){
            playMIDI();

        }else {
            //如果电平是停止电平 ，表示已经抬起按键，开始停止计时(就是按起按键后再播放一小段时间)
            playDelayer->setAllowStart(true);
        }
    }
   //每次循环都要检测是否可以停止发音了。 
    delayStopMIDI();

}
