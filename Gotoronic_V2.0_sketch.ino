//Copyright (c) 2016 goloveski
//Released under the MIT license
//http://opensource.org/licenses/mit-license.php
//
// Fメカ操作時にリヤが動くのを修正
// 160222各ギヤごとの微調整用オフセットデータを持たせる。-> pos_data[10] 
// 160222 save gear position to EEPROM
// 160222 added single LINE 2 signal SW 
// 160222 added EEPROM status limit

// SERVO REAR
// Red = +6v
// Brown = GND
// Orange = Signal (pin 9 for this code)

// SERVO FRONT
// Red = +6v
// Brown = GND
// Orange = Signal (pin 8 for this code)

// single LINE 2 signal SW system
// shift UP   SIGNAL(PIN4)SW--35Kohm--GND about 1/2 VDD
// shift DOWN SIGNAL(PIN4)SW-- 0ohm --GND 
// PIN4 
// tune adc_val, ana_read_dly, for sensitivity


#include <EEPROM.h>
#include <Servo.h> 
#include <Bounce2.h>
#include <MsTimer2.h> 


#define BUTTON_PIN 4  //REAR 
#define BUTTON_PIN2 5 //REAR
#define BUTTON_PIN_f1 6  //FRONT
#define BUTTON_PIN_f2 7  //FRONT
#define LED_PIN2 10
#define LED_PIN 13          // LED goes off when signal is received

// Instantiate a Bounce object
//Bounce debouncer = Bounce(); 
//Bounce debouncer2 = Bounce(); 
Bounce debouncer3 = Bounce(); 
Bounce debouncer4 = Bounce();

// The current buttonState
// 0 : released
// 1 : pressed less than 1 seconds
// 2 : pressed longer than 1 seconds
// 3 : pressed longer than 2 seconds
int buttonState; 
int buttonState2; 
int buttonState3; 
int buttonState4; 
unsigned long buttonPressTimeStamp;
unsigned long buttonPressTimeStamp2;
unsigned long buttonPressTimeStamp3;
unsigned long buttonPressTimeStamp4;



//ロータリーエンコーダ用の設定
volatile int state = 0;  //ロータリーエンコーダで操作する値
volatile boolean Flag_A=true;  //ロータリーエンコーダA端子の状態変化を許すフラグ
volatile boolean Flag_B=false; //ロータリーエンコーダB端子の状態変化を許すフラグ
int RotEncState = LOW; //ロータリーエンコーダで入力したことを示すフラグ

Servo myservo;  // create servo object to control a servo 
Servo myservo_f; // a maximum of eight servo objects can be created 
char input[2];  //シリアル通信の文字列格納用
char str[0];    //シリアル通信の文字列格納用
int pos = 0;    // variable to store the servo position
int pos_f = 0;  // variable to store the servo position 
int analogvalue1; //ボタン１のアナログ値
int analogvalue2; //ボタン１のアナログ値
int adc_val[2] ={400, 700}; //AD閾値
char adc_val_position[3][5] = {"LOW", "MID","HIGH"};//AD閾値
int NUM_ADC_DIV = 2;//AD閾値
int SWstatus = 0;//ADステータス
int SWstatus_new = 0;//ADステータス
int SWstatus_old = 0;//ADステータス
int SW_short_status_new = 0;//ADステータス
int SW_short_status_old = 0;//ADステータス
int ana_read_dly = 10; //delay anarog read[ms]

//////////////////////////////// EEPROM PARAMETER START ///////////////////////////////////////////
int pos_final_addres = 10; //final gear position save address
int pos_final; //final gear position
int pos_final_addres_f = 1; //final gear position save address front
int pos_final_f; //final gear position front


//////////////////////////////// REAR SHIFT SETTING PARAMETER START ///////////////////////////////////////////

int pos_zero = 2180;          // shift 0 position 180deg ->1st gear direction, 0deg ->10th gear direction, 
int pos_shiftstep = 56;      // shift step degree 68->56 @150809
int pos_data[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 10speed tuninng data

int pos_shiftover_up = 40;   // shift over degree
int pos_shiftover_down = 40; // shift over degree
int ovsft_delay = 500;      // over shift delay [msec]
int sft_delay = 20;         // shift delay per gear pos [msec]

//////////////////////////////// REAR SHIFT SETTING PARAMETER END ///////////////////////////////////////////

int pos_old;           // gear position
int pos_new;           // gear position
int pos_direction  = 1;     // shift direction
int ovsft_delay_tot = 100;    // over shift delay msec


//////////////////////////////// FRONT SHIFT SETTING PARAMETER START  ///////////////////////////////////////////

int pos_zero_f = 80;          // shift 0 position 180deg ->1st gear direction, 0deg ->10th gear direction, 
int pos_inner = 70;      //80->70 @150809
int pos_outer = 150;     //120->120 @150809
int pos_shiftstep_f = 5;      // shift step degree
int pos_shiftover_up_f = 40;   // shift over degree 15->20 @150809
int pos_shiftover_down_f = 40; // shift over degree 15->40 @150809
int ovsft_delay_f = 2000;      // over shift delay [msec]
int sft_delay_f = 20;         // shift delay per gear pos [msec]

//////////////////////////////// FONT SHIFT SETTING PARAMETER END ///////////////////////////////////////////

int pos_old_f  = 0;           // gear position
int pos_new_f  = 0;           // gear position
int pos_direction_f  = 1;     // shift direction
int ovsft_delay_tot_f = 100;    // over shift delay msec

 
void setup() 
{ 
 
  pos_final = EEPROM.read(pos_final_addres);
  if (pos_final <  0){ pos_final = 0;} //ギヤポジション0より小を禁止
  if (pos_final >  9){ pos_final = 0;} //ギヤポジション9より大を禁止
  pos_new = pos_final;
  pos = pos_zero - pos_shiftstep * pos_new + pos_data[pos_new];
  delay(2);
  myservo.attach(9);        // attaches the servo on pin 9 to the servo object 
  myservo_f.attach(8);      // attaches the servo on pin 8 to the servo object 
  myservo.writeMicroseconds(pos);                // tell servo to go to position in variable 'pos'
  delay(10);                         // waits 15ms for the servo to reach the position 15
  pos_f = pos_zero_f;         //set zero position
  myservo_f.write(pos_f);                // tell servo to go to position in variable 'pos'
  delay(200);                         // waits 15ms for the servo to reach the position 15
  Serial.begin(9600);
     // while (!Serial) {
     //   ; // wait for serial port to connect. Needed for Leonardo only
     // }
  //  establishContact();  // send a byte to establish contact until receiver responds 
   //  MsTimer2::set(1000, servoctl);
   //  MsTimer2::start();
   
  // Setup the button
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(BUTTON_PIN2,INPUT_PULLUP);
  pinMode(BUTTON_PIN_f1,INPUT);
  pinMode(BUTTON_PIN_f2,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN_f1,HIGH);
  digitalWrite(BUTTON_PIN_f2,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer3.attach(BUTTON_PIN_f1);
  debouncer4.attach(BUTTON_PIN_f2);
  debouncer3.interval(2);
  debouncer4.interval(2);
  //Setup the LED
  pinMode(LED_PIN2,OUTPUT);

  pinMode(2, INPUT_PULLUP);  //デジタルピン2をプルアップ入力にする
  pinMode(3, INPUT_PULLUP);  //デジタルピン3をプルアップ入力にする
  attachInterrupt(0, Fall_A  , FALLING);//デジタルピン2の電圧が下がった時の処理関数を登録
  attachInterrupt(1, Change_B, CHANGE );//デジタルピン3の電圧が変化した時の処理関数を登録
   
} 
 
void loop() {  
  delay(5);

  if (Serial.available() > 0) {     // get incoming byte:
  serialin();                        //下記のvoid serialinの内容を実行
  Serial.println(pos);              // send a capital ASerial.println('pos');   // send a capital pos
  myservo.writeMicroseconds(pos);                // tell servo to go to position in variable 'pos'
  myservo_f.write(pos_f);              // tell servo to go to position in variable 'pos'
  delay(50);                         // waits 15ms for the servo to reach the position 15
  Serial.println(myservo.read());
  Serial.println(myservo_f.read());
  }

//   Serial.print("START POSITION = ");
//   Serial.println(pos_final);

 // Update the debouncer and get the changed state
  boolean changed_f1 = debouncer3.update();
  boolean changed_f2 = debouncer4.update();


 
 //Read Analog SW   
   int n;
   for (n = 0; n < 10; n++){
      if (SW_short_status_old != SW_short_status_new){
          SW_short_status_old = SW_short_status_new;
          delay(10); 
          break;
          }
          
       ReadAna1() ;
       int k;
       for (k = 0; k < NUM_ADC_DIV; k++){
            if (analogvalue1 < adc_val[k]){
               break;
               }   
       }
       SW_short_status_new = k;
       k = 0;
     }
   SWstatus_new = SW_short_status_new; 
//   Serial.print("ANALOG SW POSITION = ");
//   Serial.println( adc_val_position[SWstatus_new]);
   delay(10); 

if (SWstatus_old != SWstatus_new){
      SWstatus_old = SWstatus_new;

        switch(SWstatus_new){
            case 0:   
               digitalWrite(LED_PIN2, LOW );
               buttonState2 = 1;
               buttonState = 0;
               Serial.println("Button pressed (state LOW)");
               buttonPressTimeStamp2 = millis();
               pos_old = pos_new;
               if( pos_new > 0 ) {pos_new --;}
               pos_ctrl();
               break;

            case 1:
               digitalWrite(LED_PIN2, LOW );
               buttonState = 1;
               buttonState2 = 0;
               Serial.println("Button pressed (state MID)");
               buttonPressTimeStamp = millis();
               pos_old = pos_new;
               if( pos_new < 9 ) {pos_new ++;}
               pos_ctrl();
               break;  
    
            case 2:
               // Get the update value
               digitalWrite(LED_PIN2, HIGH );
               buttonState = 0;
               buttonState2 = 0;
               Serial.println("Button released (state HIGH)");
               pos_ctrl_end();
               break;    
        } 
   }
   
 /* 多段変速用*/
 
  if  ( buttonState == 1 ) {
    if ( millis() - buttonPressTimeStamp >= 500 ) {
        buttonState = 2;
        Serial.println("Button held for 0.50 seconds (state 2)");
        pos_old = pos_new;
        if( pos_new < 9 ) {pos_new ++;}
        pos_ctrl();
       }
  }
  if  ( buttonState == 2 ) {
    if ( millis() - buttonPressTimeStamp >= 1000 ) {
        buttonState = 3;
        Serial.println("Button held for 1.0 seconds (state 3)");
        pos_old = pos_new;
        if( pos_new < 9 ) { pos_new ++;}
        pos_ctrl();
        }
  } 
   if  ( buttonState == 3 ) {
    if ( millis() - buttonPressTimeStamp >= 150 ) {
        buttonState = 0;
       Serial.println("Button held for 1.5 seconds (state 4)");
       pos_old = pos_new;
       if( pos_new < 9 ) {pos_new ++;}
           pos_ctrl();
    }
  } 
 
  if  ( buttonState2 == 1 ) {
    if ( millis() - buttonPressTimeStamp2 >= 500 ) {
        buttonState2 = 2;
       Serial.println("Button2 held for 0.50 seconds (state 2)");
       pos_old = pos_new;
       if( pos_new  >0 ) {pos_new --;}
           pos_ctrl();
       }
  }
  if  ( buttonState2 == 2 ) {
    if ( millis() - buttonPressTimeStamp2 >= 1000 ) {
        buttonState2 = 3;
       Serial.println("Button2 held for 1.0 seconds (state 3)");
       pos_old = pos_new;
       if( pos_new  >0 ) {pos_new --;}
           pos_ctrl();
       }
  } 
  if  ( buttonState2 == 3 ) {
    if ( millis() - buttonPressTimeStamp2 >= 1500 ) {
        buttonState2 = 0;
       Serial.println("Button2 held for 1.5 seconds (state 4)");
       pos_old = pos_new;
       if( pos_new  >0 ) { pos_new --;}
           pos_ctrl();
       }
  } 


  
if ( changed_f1 ) {
     // Get the update value
  int value = debouncer3.read();
  if ( value == HIGH) {
     digitalWrite(LED_PIN2, HIGH );
     buttonState3 = 0;
     Serial.println("Button released (state 0)");
     pos_f = pos_inner + pos_shiftover_down_f;   // shift over degree 
     Serial.println(pos_f);
     myservo_f.write(pos_f); 
     
 }
 else  {
       digitalWrite(LED_PIN2, LOW );
       buttonState3 = 1;
       Serial.println("Button pressed (state 1)");
       buttonPressTimeStamp3 = millis();
       pos_old_f = pos_new_f;
       //if( pos_new < 9 ) {pos_new ++;}
       pos_new_f = pos_inner;
       pos_f = pos_inner;   // shift over degree 
       Serial.println(pos_f);
       myservo_f.write(pos_f); 
   }
}

if ( changed_f2 ) {
     // Get the update value
  int value2 = debouncer4.read();
  if ( value2 == HIGH) {
     digitalWrite(LED_PIN2, HIGH );
     buttonState4 = 0;
     Serial.println("Button2 released (state 0)");
     pos_f = pos_outer - pos_shiftover_up_f;  // shift over degree
     Serial.println(pos_f);
     myservo_f.write(pos_f);   
 }
 else {
       digitalWrite(LED_PIN2, LOW );
       buttonState4 = 1;
       Serial.println("Button2 pressed (state 1)");
       buttonPressTimeStamp4 = millis();
       pos_old_f = pos_new_f;
     //  if( pos_new >0 ) {pos_new --;}
       pos_new_f = pos_outer;
       pos_f = pos_outer;  // shift over degree
       Serial.println(pos_f);
       myservo_f.write(pos_f);
      }
}


} // LOOP 完了



//////////////SUB CKT/////////////////////////

void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println('A');   // send a capital A
    delay(300);
  }
}

void serialin() {
    Serial.print("serial in" );
    for(int i=0;i<=2;i++)//iが0～2まで変動するので、合計3桁分
        {
//          Serial.println(i);
          input[i]=Serial.read(); //一桁づつ入れてゆく
//          delay(100); 
        }
       Serial.println(input); 
       int buf=atoi(input);//シリアル入力された文字列をint型に変換
//       if(buf<=180&&buf>=0)//安全のため、PWMで扱える0～255の範囲の時のみPWM出力の値に反映
//       {
         pos=buf;
        Serial.print("serial input = ");
        Serial.println(pos);
//       }
 
//      Serial.flush();  
 }

///////////////READ ANALOG///////////////////
void ReadAna1() {
    analogvalue1 = analogRead(A6);
//      Serial.print("analogvalue1 = ");
//      Serial.println(analogvalue1);
    delay(ana_read_dly);  //wait ana_read_dly msec
    }

///////////////POSITION CTRL///////////////////
void pos_ctrl() 
{
        pos_direction = pos_new - pos_old;
        ovsft_delay_tot = abs(pos_direction) * sft_delay + ovsft_delay;
//        Serial.println(ovsft_delay_tot);
        if( pos_direction > 0 ) {                                         // up stroke
           pos = pos_zero - pos_shiftstep * pos_new + pos_data[pos_new] - pos_shiftover_up;   // shift over degree
           Serial.print("servo position = ");
           Serial.println(pos);
//delay(130);
           myservo.writeMicroseconds(pos); 
//           delay(ovsft_delay_tot);                                         // waits Xmsec for the servo to reach the position 
//           pos = pos_zero - pos_shiftstep * pos_new;                       // shift over degree
//           myservo.writeMicroseconds(pos);                                             // tell servo to go to position in variable 'pos'
        }
       if( pos_direction < 0 ) {                                           // down stroke 
           pos = pos_zero - pos_shiftstep * pos_new + pos_data[pos_new] + pos_shiftover_down;  // shift over degree
           Serial.print("servo position = ");
           Serial.println(pos);
// delay(130);
           myservo.writeMicroseconds(pos); 
//           delay(ovsft_delay_tot);                                         // waits Xmsec for the servo to reach the position 
//           pos = pos_zero - pos_shiftstep * pos_new;                       // shift over degree
//           myservo.writeMicroseconds(pos);                                             // tell servo to go to position in variable 'pos'
        }
//       Serial.println(pos);
//       Serial.println(myservo.read());
//       delay(15);                                                          // waits 15ms for the servo to reach the position 
   if( pos_new == 9 ) {                                         // 9position
           pos = pos_zero - pos_shiftstep * pos_new + pos_data[pos_new] - pos_shiftover_up;   // shift over degree
           myservo.writeMicroseconds(pos); 
   }
   if( pos_new == 0 ) {                                         // 9position
           pos = pos_zero - pos_shiftstep * pos_new + pos_data[pos_new] + pos_shiftover_down;    // shift over degree
           myservo.writeMicroseconds(pos); 
   }


}

void pos_ctrl_end() {
           pos = pos_zero - pos_shiftstep * pos_new + pos_data[pos_new];                       // shift over degree        
           myservo.writeMicroseconds(pos);                                             // tell servo to go to position in variable 'pos' 
           pos_final = EEPROM.read(pos_final_addres);
 //          delay(1000);
          // Serial.print("EEPROM OLD = ");
          // Serial.println(pos_final);
           pos_final = pos_new;
           EEPROM.write(pos_final_addres, pos_final);
           delay(5);
           Serial.print("servo position = ");
           Serial.println(pos);
           //Serial.println(myservo.read());
           Serial.print("gear position = ");
           Serial.println(pos_new);
           pos_final = EEPROM.read(pos_final_addres);
           Serial.print("EEPROM NEW = ");
           Serial.println(pos_final);
           
}


///////////////POSITION CTRL FRONT///////////////////
void servo_move()
{
  myservo_f.write(pos_f);
      MsTimer2::stop();
      Serial.println("TimerDelay");
      Serial.println(myservo_f.read());
}


void pos_ctrl_f() 
{
        pos_direction_f = pos_new_f - pos_old_f;
        ovsft_delay_tot_f = abs(pos_direction_f) * sft_delay_f + ovsft_delay_f;
        Serial.println(ovsft_delay_tot_f);
        if( pos_direction_f > 0 ) {                                         // up stroke
           pos_f = pos_inner - pos_shiftover_up_f;   // shift over degree 
           Serial.println(pos_f);
// delay(130);
           myservo_f.write(pos_f); 
           //delay(ovsft_delay_tot_f);                                         // waits Xmsec for the servo to reach the position 
           if( RotEncState = HIGH){
           RotEncState = LOW;  
           pos_f = pos_inner;                       // shift over degree
           MsTimer2::set(ovsft_delay_tot_f,servo_move); // 500msごとにオンオフ
           MsTimer2::start();
           // myservo_f.write(pos_f);                                             // tell servo to go to position in variable 'pos'
           }
        }
       if( pos_direction_f < 0 ) {                                           // down stroke 
           pos_f = pos_outer + pos_shiftover_down_f;  // shift over degree
           Serial.println(pos_f);
// delay(130);
           myservo_f.write(pos_f); 
           //delay(ovsft_delay_tot_f);            // waits Xmsec for the servo to reach the position 
          if( RotEncState = HIGH){
           RotEncState = LOW;  
           pos_f = pos_outer;                       // shift over degree
           MsTimer2::set(ovsft_delay_tot_f,servo_move); // 500msごとにオンオフ
           MsTimer2::start();
           //myservo_f.write(pos_f);                                             // tell servo to go to position in variable 'pos'
          }
        }
       Serial.println(pos_f);

       delay(1);                                                          // waits 15ms for the servo to reach the position 
}

//void pos_ctrl_end_f() {
//       //pos_f = pos_zero_f - pos_shiftstep_f * pos_new_f;
//       pos_f = pos_zero_f;
//       myservo_f.writeMicroseconds(pos_f);                                             // tell servo to go to position in variable 'pos' myservoy->myservo_f 2015/2/26     
//       Serial.println(myservo_f.read());
//       Serial.print("Front gear position = ");
//       Serial.println(pos_new_f); 
//}

///////////////ROT_ENC CTRL FRONT///////////////////
void Fall_A() {  //デジタルピン2の電圧が下がった時

if(!Flag_A){return;}  //A端子の認識が許されない時は戻る
Flag_B=true;  //B端子の状態変化の監視を有効にする
Flag_A=false; //A端子の状態変化の監視を無効にする

}
void Change_B() {  //デジタルピン3の電圧が変化した時
  if(!Flag_B){return;} //B端子の認識が許されない時は戻る
  if(HIGH == digitalRead(2)){ //既にA端子がHIGHならば
        if(HIGH == digitalRead(3)){
          state--;
                  Serial.println(state);  //シリアルで値を送信
          pos_old_f = pos_new_f;
          pos_new_f = pos_inner;
          pos_ctrl_f();
          }else{
          state++;
            Serial.println(state);  //シリアルで値を送信
          pos_old_f = pos_new_f;
          pos_new_f = pos_outer;
          RotEncState = HIGH;     //ロータリーエンコーダで入力したことを示すフラグ
          pos_ctrl_f();
          }  //現在の電圧から加減算を行う
        Flag_B=false; //B端子の状態変化の監視を無効にする
        Flag_A=true;  //A端子の状態変化の監視を有効にする

        }

}



