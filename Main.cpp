// Master側プログラム

#include <math.h>
#include "mbed.h"
#include "rtos.h"
#include "ACM1602NI.h"
#include "Ping.h"

#define PI 3.14159265358979 // 円周率πの定義




// ピンの設定
    DigitalOut led1(LED1);
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);
    DigitalOut led4(LED4);

    // フルカラーLED
    DigitalOut Led_R(p12);
    DigitalOut Led_G(p13);
    DigitalOut Led_B(p14);

    // 操作パネル
    InterruptIn SW1(p23);
    DigitalOut SW2(p22);
    DigitalOut SW3(p21);


    // モータの制御
    PwmOut pwm1(p24);
    PwmOut pwm2(p25);
    PwmOut pwm3(p26);
    DigitalOut M1_1(p15), M1_2(p16);
    DigitalOut M2_1(p17), M2_2(p18);
    DigitalOut M3_2(p19), M3_1(p20);

    // キッカーの制御
    Timer Kicker_Timer;
    DigitalOut shout(p30);
    DigitalOut charge(p29);

    // ラインセンサの信号ピン
    DigitalIn LINE_F(p7);
    DigitalIn LINE_B(p5);
    DigitalIn LINE_R(p6);
    DigitalIn LINE_L(p8);

    // それぞれのセンサとの通信
    I2C I2C_mbed(p28, p27);
    I2C LCD(p9 , p10);
    ACM1602NI lcd(LCD);



// プロトタイプ宣言
    void SW_Start(void);

    /*---- RotarySW.h ----*/
        void FW(void);
        void DEBUG_IR(void);
        void DEBUG_PING(void);
        void DEBUG_ANGLE(void);
        void DEBUG_KICKER(void);
        void DEBUG_MOTER(void);
        void DEBUG_LINE(void);

    /*---- I2C_Master.h ----*/
        void MBED_PING(void);
        void MBED_IR(void);
        void MBED_LCD(void);
        void MBED_MODE(void);

    /*---- Moter.h ----*/
        void Moter(float speed, int angle, float omega);
        void MoterApi(bool m1, bool m2, bool m3);
        void MoterReset (bool m1, bool m2, bool m3);
        float Auto_Corrction(void);

    /*---- HMC6352.h ----*/
        void COMPASS_RESET(void);
        void COMPASS(void);

    /*---- ModeChange.h ----*/
        void MODE(char mode);




void SW_Start(void) {
    sw_flag = !sw_flag;
}


int main() {
    /*---- 初期化処理 ----*/
    SW1.rise(&SW_Start);
    MoterReset(0, 0, 0);
    COMPASS_RESET();
    wait(0.7);

    /*---- modeを確認 ----*/
    MBED_MODE(); wait(1.0);

    /*---- 各モードに移行 ----*/
    lcd.locate(0, 0);
    switch(Mode[0]) {
        case 0: lcd.printf("FW");              while(1) FW();
        case 2: lcd.printf("DEBUG_IR");        while(1) DEBUG_IR();
        case 3: lcd.printf("DEBUG_PING");      while(1) DEBUG_PING();
        case 4: lcd.printf("DEBUG_ANGLE");     while(1) DEBUG_ANGLE();
        case 5: lcd.printf("DEBUG_KICKER");    while(1) DEBUG_KICKER();
        case 6: lcd.printf("DEBUG_MOTER");     while(1) DEBUG_MOTER();
        case 7: lcd.printf("DEBUG_LINE");      while(1) DEBUG_LINE();
        case 8: lcd.printf("FW");              while(1) FW();
        case 9: lcd.printf("FW");              while(1) FW();
        default: return 0;
    }
}







// モード
    void MODE(char mode) {
        switch(mode) {
            case 0:  while(1){ FW();            } break;
            case 1:  while(1){ DF();            } break;
            case 2:  while(1){ IR();            } break;
            case 3:  while(1){ PING();          } break;
            case 4:  while(1){ ORIENTATION();   } break;
            case 5:  while(1){ KICKER();        } break;
            case 6:  while(1){ DRIBBLER();      } break;
            case 7:  while(1){ MOTER();         } break;
            case 8:  while(1){ LINE();          } break;
            case 9:  break;
            default:　break;
        }
    }



// RotarySW
    // "0"攻撃モード
    void FW(void) {
        if(sw_flag) {
        led1=0; led2=0; led3=0; led4=0;
        Speed = 1.0;
        MBED_IR();
        COMPASS();

        // Kickerの制御
            /*
            if(!Kicker_Flag) {                  // Chargeの開始
                Kicker_Timer.reset();
                shout=0;  charge=1;  Kicker_Timer.start();
                Kicker_Flag = 1;
            } else {                            // Charge時間を確認
                // Kicker_Timer.stop();
                if (Kicker_Timer.read() > 10.0) {
                    charge = 0;
                    Kicker_Flag = 2;
                }
            }
            if(BallCheck && Kicker_Flag==2) {   // Kick可能な時の処理
                int ping_width = PING_R + PING_L;
                if (ping_width>150) {
                    ping_width = PING_R - PING_L;
                    if (ping_width> 90) Moter(1.0, Angle,  0.1);
                    if (ping_width<-90) Moter(1.0, Angle, -0.1);
                }
                charge=0;  shout=1;  wait(0.3);
                Kicker_Flag = 0;
            }
            */
        Moter(Speed, Angle, Auto_Corrction());

        // if (10.0 < Compass && Compass < 180.0) {
        //     MoterApi(0,0,0);
        //     pwm1=0.3; pwm2=0.3; pwm3=0.3;
        // }
        // if (-180.0 < Compass && Compass < -10.0) {
        //     MoterApi(1,1,1);
        //     pwm1=0.3; pwm2=0.3; pwm3=0.3;
        // }

        if (Angle == 001) {
            pwm1 = 0;
            pwm2 = 0;
            pwm3 = 0;
        }

        // LINEの制御
            /*
            if(LINE_R || LINE_L)  Angle = 360 - Angle;
            else if(LINE_F)  Angle = 180;
            else if(LINE_B)  Angle = 0;
            */
            if(LINE_F || LINE_R || LINE_B || LINE_L) {
                MoterApi(1, 0, 0);
                pwm1 = 0.5;
                pwm2 = 0.5;
                pwm3 = 0;
            }
        }
    }




    // "2"IRデバッグモード
    void DEBUG_IR(void) {
        char ir[2];
        do val = I2C_mbed.read(mbed_slave, ir, 2); while(val);
        // LCDに表示
            lcd.locate(0,0);  lcd.printf("near: ");
            lcd.locate(0,1);  lcd.printf("long: ");
            for (int i = 0; i < 8; ++i) {
                lcd.locate(i+6, 0);  lcd.printf("%1d", ir[0]%2);  ir[0] = ir[0] / 2;
                lcd.locate(i+6, 1);  lcd.printf("%1d", ir[1]%2);  ir[1] = ir[1] / 2;
            }
        led1=0; led2=0; led3=0; led4=0;
    }


    // "3"PINGデバッグモード
    void DEBUG_PING(void) {
        char ping[4];
        val = I2C_mbed.read(mbed_slave, ping, 4);
        if(!val) led1 = 1;
        // LCDに表示
            lcd.locate(0,0);  lcd.printf("F:%04.1f  ", ping[0]/10);
            lcd.locate(8,0);  lcd.printf("R:%04.1f  ", ping[1]/10);
            lcd.locate(0,1);  lcd.printf("L:%04.1f  ", ping[3]/10);
            lcd.locate(8,1);  lcd.printf("B:%04.1f  ", ping[2]/10);
        led1=0; led2=0; led3=0; led4=0;
    }

    // "4"ANGLEデバッグモード
    void DEBUG_ANGLE(void) {
        COMPASS();
        lcd.locate(8, 1);
        lcd.printf("%05.1f", Compass);
        led1=0; led2=0; led3=0; led4=0;
    }

    // "5"KICKERデバッグモード
    void DEBUG_KICKER(void) {
        shout=0;  charge=1;  wait(3.0);
        shout=0;  charge=0;  wait(0.5);
        // while(!SW1);
        shout=1;  charge=0;  wait(0.5);
        led1=0; led2=0; led3=0; led4=0;
    }

    // "6"MOTERデバッグモード
    void DEBUG_MOTER(void) {
        for (int i = 0; i < 4; ++i) {
            Moter(1.0, i*90, 0.0);  wait(0.5);
            switch(i) {
                case 0:
                    Moter(1.0, 2*90, 0.0);  wait(0.5);  break;
                case 1:
                    Moter(1.0, 3*90, 0.0);  wait(0.5);  break;
                case 2:
                    Moter(1.0, 0*90, 0.0);  wait(0.5);  break;
                case 3:
                    Moter(1.0, 1*90, 0.0);  wait(0.5);  break;
            }
        }
        led1=0; led2=0; led3=0; led4=0;
    }

    // "7"LINEデバッグモード
    void DEBUG_LINE(void) {
        // LCDに表示
            lcd.locate(0,0);  lcd.printf("F:%1d     ", (LINE_F)?1:0);
            lcd.locate(8,0);  lcd.printf("R:%1d     ", (LINE_R)?1:0);
            lcd.locate(0,1);  lcd.printf("L:%1d     ", (LINE_L)?1:0);
            lcd.locate(8,1);  lcd.printf("B:%1d     ", (LINE_B)?1:0);
        led1=0; led2=0; led3=0; led4=0;
    }



    void (*Debug_Mode[])(void) = {FW, DF, DEBUG_IR, DEBUG_PING, DEBUG_ANGLE, DEBUG_KICKER, DEBUG_MOTER, DEBUG_LINE};



// HMC6352
    // 基準角度を求める
    void COMPASS_RESET(void) {
        Compass_mode[0] = 0x47;
        Compass_mode[1] = 0x74;
        Compass_mode[2] = 0x72;
        I2C_mbed.write(compass_addr, Compass_mode, 3);

        float compass = 0.0;
        for (int i=0; i<10; i++) {
            val = I2C_mbed.read(compass_addr, Compass_mode, 2);
            if(!val) led4 = 1;
            compass += 0.1 * ((Compass_mode[0] << 8) + Compass_mode[1]);
        }
        Compass_Base = compass / 10.0;
        lcd.locate(0, 1);  lcd.printf("%5.1f", Compass_Base);
    }


    // 現在の向いている方向を求める
    void COMPASS(void) {
        Compass = 0.0;
        val = I2C_mbed.read(compass_addr, Compass_mode, 2);
        Compass = 0.1 * ((Compass_mode[0] << 8) + Compass_mode[1]);
        Compass -= Compass_Base;

        if (Compass >=  180.0) Compass -= 360.0;
        if (Compass <  -180.0) Compass += 360.0;

        if(!val)led1 = 1;   else led1 = 0;
    }



// mbedSlaveとのI2C通信用プログラム
    // まず欲しいデータを教えてから、欲しいデータを受け取る
    void MBED_PING(void) {
        val = I2C_mbed.write(mbed_slave, Order+0, 1);   // PINGデータを要求
        val = I2C_mbed.read(mbed_slave, Rec, 4);        // PINGデータを受信
        PING_F = Rec[0];
        PING_R = Rec[1];
        PING_B = Rec[2];
        PING_L = Rec[3];
        if(!val)led2 = 1;   else led2 = 0;
    }


    void MBED_IR(void) {
        val = I2C_mbed.write(mbed_slave, Order+1, 1);       // IRデータを要求
        val = I2C_mbed.read(mbed_slave, Rec, 4);            // IRデータを受信
        Angle = Rec[0] + Rec[1];    // 進行方向
        Speed = (float)(Rec[2]) / 100.00000;        // 速度
        BallCheck = Rec[3];         // ボール保持
        if(!val) led2 = 1;   else led2 = 0;
    }


    void MBED_LCD(void) {
        val = I2C_mbed.write(mbed_slave, Order+2, 1);       // LCDデータを要求
        val = I2C_mbed.read(mbed_slave, Rec, 17);           // LCDデータを受信
        lcd.printf("%s\n", Rec);
        if(!val)led2 = 1;   else led2 = 0;
    }


    void MBED_MODE(void) {
        val = I2C_mbed.write(mbed_slave, Order+3, 1);       // modeデータを要求
        val = I2C_mbed.read(mbed_slave, Mode, 1);           // modeデータを受信
        if(!val)led2 = 1;   else led2 = 0;
    }



// モータ制御
    // モータのPWM制御（１°刻み）
        void M000(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.00000;}
        void M001(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.03023;}
        void M002(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.06048;}
        void M003(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.09077;}
        void M004(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.12112;}
        void M005(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.15153;}
        void M006(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.18205;}
        void M007(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.21267;}
        void M008(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.24342;}
        void M009(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.27433;}
        void M010(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.30541;}
        void M011(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.33668;}
        void M012(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.36816;}
        void M013(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.39988;}
        void M014(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.43185;}
        void M015(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.46410;}
        void M016(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.49666;}
        void M017(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.52954;}
        void M018(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.56278;}
        void M019(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.59639;}
        void M020(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.63041;}
        void M021(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.66487;}
        void M022(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.69979;}
        void M023(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.73521;}
        void M024(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.77116;}
        void M025(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.80767;}
        void M026(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.84478;}
        void M027(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.88252;}
        void M028(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.92095;}
        void M029(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-0.96009;}
        void M030(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M031(void)   {PWM1=-0.96009;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M032(void)   {PWM1=-0.92095;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M033(void)   {PWM1=-0.88252;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M034(void)   {PWM1=-0.84478;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M035(void)   {PWM1=-0.80767;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M036(void)   {PWM1=-0.77116;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M037(void)   {PWM1=-0.73521;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M038(void)   {PWM1=-0.69979;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M039(void)   {PWM1=-0.66487;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M040(void)   {PWM1=-0.63041;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M041(void)   {PWM1=-0.59639;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M042(void)   {PWM1=-0.56278;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M043(void)   {PWM1=-0.52954;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M044(void)   {PWM1=-0.49666;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M045(void)   {PWM1=-0.46410;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M046(void)   {PWM1=-0.43185;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M047(void)   {PWM1=-0.39988;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M048(void)   {PWM1=-0.36816;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M049(void)   {PWM1=-0.33668;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M050(void)   {PWM1=-0.30541;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M051(void)   {PWM1=-0.27433;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M052(void)   {PWM1=-0.24342;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M053(void)   {PWM1=-0.21267;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M054(void)   {PWM1=-0.18205;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M055(void)   {PWM1=-0.15153;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M056(void)   {PWM1=-0.12112;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M057(void)   {PWM1=-0.09077;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M058(void)   {PWM1=-0.06048;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M059(void)   {PWM1=-0.03023;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M060(void)   {PWM1= 0.00000;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M061(void)   {PWM1= 0.03023;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M062(void)   {PWM1= 0.06048;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M063(void)   {PWM1= 0.09077;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M064(void)   {PWM1= 0.12112;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M065(void)   {PWM1= 0.15153;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M066(void)   {PWM1= 0.18205;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M067(void)   {PWM1= 0.21267;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M068(void)   {PWM1= 0.24342;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M069(void)   {PWM1= 0.27433;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M070(void)   {PWM1= 0.30541;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M071(void)   {PWM1= 0.33668;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M072(void)   {PWM1= 0.36816;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M073(void)   {PWM1= 0.39988;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M074(void)   {PWM1= 0.43185;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M075(void)   {PWM1= 0.46410;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M076(void)   {PWM1= 0.49666;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M077(void)   {PWM1= 0.52954;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M078(void)   {PWM1= 0.56278;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M079(void)   {PWM1= 0.59639;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M080(void)   {PWM1= 0.63041;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M081(void)   {PWM1= 0.66487;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M082(void)   {PWM1= 0.69979;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M083(void)   {PWM1= 0.73521;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M084(void)   {PWM1= 0.77116;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M085(void)   {PWM1= 0.80767;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M086(void)   {PWM1= 0.84478;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M087(void)   {PWM1= 0.88252;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M088(void)   {PWM1= 0.92095;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M089(void)   {PWM1= 0.96009;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M090(void)   {PWM1= 1.00000;   PWM2= 1.00000;   PWM3=-1.00000;}
        void M091(void)   {PWM1= 1.00000;   PWM2= 0.96009;   PWM3=-1.00000;}
        void M092(void)   {PWM1= 1.00000;   PWM2= 0.92095;   PWM3=-1.00000;}
        void M093(void)   {PWM1= 1.00000;   PWM2= 0.88252;   PWM3=-1.00000;}
        void M094(void)   {PWM1= 1.00000;   PWM2= 0.84478;   PWM3=-1.00000;}
        void M095(void)   {PWM1= 1.00000;   PWM2= 0.80767;   PWM3=-1.00000;}
        void M096(void)   {PWM1= 1.00000;   PWM2= 0.77116;   PWM3=-1.00000;}
        void M097(void)   {PWM1= 1.00000;   PWM2= 0.73521;   PWM3=-1.00000;}
        void M098(void)   {PWM1= 1.00000;   PWM2= 0.69979;   PWM3=-1.00000;}
        void M099(void)   {PWM1= 1.00000;   PWM2= 0.66487;   PWM3=-1.00000;}
        void M100(void)   {PWM1= 1.00000;   PWM2= 0.63041;   PWM3=-1.00000;}
        void M101(void)   {PWM1= 1.00000;   PWM2= 0.59639;   PWM3=-1.00000;}
        void M102(void)   {PWM1= 1.00000;   PWM2= 0.56278;   PWM3=-1.00000;}
        void M103(void)   {PWM1= 1.00000;   PWM2= 0.52954;   PWM3=-1.00000;}
        void M104(void)   {PWM1= 1.00000;   PWM2= 0.49666;   PWM3=-1.00000;}
        void M105(void)   {PWM1= 1.00000;   PWM2= 0.46410;   PWM3=-1.00000;}
        void M106(void)   {PWM1= 1.00000;   PWM2= 0.43185;   PWM3=-1.00000;}
        void M107(void)   {PWM1= 1.00000;   PWM2= 0.39988;   PWM3=-1.00000;}
        void M108(void)   {PWM1= 1.00000;   PWM2= 0.36816;   PWM3=-1.00000;}
        void M109(void)   {PWM1= 1.00000;   PWM2= 0.33668;   PWM3=-1.00000;}
        void M110(void)   {PWM1= 1.00000;   PWM2= 0.30541;   PWM3=-1.00000;}
        void M111(void)   {PWM1= 1.00000;   PWM2= 0.27433;   PWM3=-1.00000;}
        void M112(void)   {PWM1= 1.00000;   PWM2= 0.24342;   PWM3=-1.00000;}
        void M113(void)   {PWM1= 1.00000;   PWM2= 0.21267;   PWM3=-1.00000;}
        void M114(void)   {PWM1= 1.00000;   PWM2= 0.18205;   PWM3=-1.00000;}
        void M115(void)   {PWM1= 1.00000;   PWM2= 0.15153;   PWM3=-1.00000;}
        void M116(void)   {PWM1= 1.00000;   PWM2= 0.12112;   PWM3=-1.00000;}
        void M117(void)   {PWM1= 1.00000;   PWM2= 0.09077;   PWM3=-1.00000;}
        void M118(void)   {PWM1= 1.00000;   PWM2= 0.06048;   PWM3=-1.00000;}
        void M119(void)   {PWM1= 1.00000;   PWM2= 0.03023;   PWM3=-1.00000;}
        void M120(void)   {PWM1= 1.00000;   PWM2= 0.00000;   PWM3=-1.00000;}
        void M121(void)   {PWM1= 1.00000;   PWM2=-0.03023;   PWM3=-1.00000;}
        void M122(void)   {PWM1= 1.00000;   PWM2=-0.06048;   PWM3=-1.00000;}
        void M123(void)   {PWM1= 1.00000;   PWM2=-0.09077;   PWM3=-1.00000;}
        void M124(void)   {PWM1= 1.00000;   PWM2=-0.12112;   PWM3=-1.00000;}
        void M125(void)   {PWM1= 1.00000;   PWM2=-0.15153;   PWM3=-1.00000;}
        void M126(void)   {PWM1= 1.00000;   PWM2=-0.18205;   PWM3=-1.00000;}
        void M127(void)   {PWM1= 1.00000;   PWM2=-0.21267;   PWM3=-1.00000;}
        void M128(void)   {PWM1= 1.00000;   PWM2=-0.24342;   PWM3=-1.00000;}
        void M129(void)   {PWM1= 1.00000;   PWM2=-0.27433;   PWM3=-1.00000;}
        void M130(void)   {PWM1= 1.00000;   PWM2=-0.30541;   PWM3=-1.00000;}
        void M131(void)   {PWM1= 1.00000;   PWM2=-0.33668;   PWM3=-1.00000;}
        void M132(void)   {PWM1= 1.00000;   PWM2=-0.36816;   PWM3=-1.00000;}
        void M133(void)   {PWM1= 1.00000;   PWM2=-0.39988;   PWM3=-1.00000;}
        void M134(void)   {PWM1= 1.00000;   PWM2=-0.43185;   PWM3=-1.00000;}
        void M135(void)   {PWM1= 1.00000;   PWM2=-0.46410;   PWM3=-1.00000;}
        void M136(void)   {PWM1= 1.00000;   PWM2=-0.49666;   PWM3=-1.00000;}
        void M137(void)   {PWM1= 1.00000;   PWM2=-0.52954;   PWM3=-1.00000;}
        void M138(void)   {PWM1= 1.00000;   PWM2=-0.56278;   PWM3=-1.00000;}
        void M139(void)   {PWM1= 1.00000;   PWM2=-0.59639;   PWM3=-1.00000;}
        void M140(void)   {PWM1= 1.00000;   PWM2=-0.63041;   PWM3=-1.00000;}
        void M141(void)   {PWM1= 1.00000;   PWM2=-0.66487;   PWM3=-1.00000;}
        void M142(void)   {PWM1= 1.00000;   PWM2=-0.69979;   PWM3=-1.00000;}
        void M143(void)   {PWM1= 1.00000;   PWM2=-0.73521;   PWM3=-1.00000;}
        void M144(void)   {PWM1= 1.00000;   PWM2=-0.77116;   PWM3=-1.00000;}
        void M145(void)   {PWM1= 1.00000;   PWM2=-0.80767;   PWM3=-1.00000;}
        void M146(void)   {PWM1= 1.00000;   PWM2=-0.84478;   PWM3=-1.00000;}
        void M147(void)   {PWM1= 1.00000;   PWM2=-0.88252;   PWM3=-1.00000;}
        void M148(void)   {PWM1= 1.00000;   PWM2=-0.92095;   PWM3=-1.00000;}
        void M149(void)   {PWM1= 1.00000;   PWM2=-0.96009;   PWM3=-1.00000;}
        void M150(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-1.00000;}
        void M151(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.96009;}
        void M152(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.92095;}
        void M153(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.88252;}
        void M154(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.84478;}
        void M155(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.80767;}
        void M156(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.77116;}
        void M157(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.73521;}
        void M158(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.69979;}
        void M159(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.66487;}
        void M160(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.63041;}
        void M161(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.59639;}
        void M162(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.56278;}
        void M163(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.52954;}
        void M164(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.49666;}
        void M165(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.46410;}
        void M166(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.43185;}
        void M167(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.39988;}
        void M168(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.36816;}
        void M169(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.33668;}
        void M170(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.30541;}
        void M171(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.27433;}
        void M172(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.24342;}
        void M173(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.21267;}
        void M174(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.18205;}
        void M175(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.15153;}
        void M176(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.12112;}
        void M177(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.09077;}
        void M178(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.06048;}
        void M179(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3=-0.03023;}
        void M180(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.00000;}
        void M181(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.03023;}
        void M182(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.06048;}
        void M183(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.09077;}
        void M184(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.12112;}
        void M185(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.15153;}
        void M186(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.18205;}
        void M187(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.21267;}
        void M188(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.24342;}
        void M189(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.27433;}
        void M190(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.30541;}
        void M191(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.33668;}
        void M192(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.36816;}
        void M193(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.39988;}
        void M194(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.43185;}
        void M195(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.46410;}
        void M196(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.49666;}
        void M197(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.52954;}
        void M198(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.56278;}
        void M199(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.59639;}
        void M200(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.63041;}
        void M201(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.66487;}
        void M202(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.69979;}
        void M203(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.73521;}
        void M204(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.77116;}
        void M205(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.80767;}
        void M206(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.84478;}
        void M207(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.88252;}
        void M208(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.92095;}
        void M209(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 0.96009;}
        void M210(void)   {PWM1= 1.00000;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M211(void)   {PWM1= 0.96009;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M212(void)   {PWM1= 0.92095;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M213(void)   {PWM1= 0.88252;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M214(void)   {PWM1= 0.84478;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M215(void)   {PWM1= 0.80767;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M216(void)   {PWM1= 0.77116;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M217(void)   {PWM1= 0.73521;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M218(void)   {PWM1= 0.69979;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M219(void)   {PWM1= 0.66487;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M220(void)   {PWM1= 0.63041;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M221(void)   {PWM1= 0.59639;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M222(void)   {PWM1= 0.56278;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M223(void)   {PWM1= 0.52954;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M224(void)   {PWM1= 0.49666;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M225(void)   {PWM1= 0.46410;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M226(void)   {PWM1= 0.43185;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M227(void)   {PWM1= 0.39988;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M228(void)   {PWM1= 0.36816;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M229(void)   {PWM1= 0.33668;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M230(void)   {PWM1= 0.30541;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M231(void)   {PWM1= 0.27433;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M232(void)   {PWM1= 0.24342;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M233(void)   {PWM1= 0.21267;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M234(void)   {PWM1= 0.18205;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M235(void)   {PWM1= 0.15153;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M236(void)   {PWM1= 0.12112;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M237(void)   {PWM1= 0.09077;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M238(void)   {PWM1= 0.06048;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M239(void)   {PWM1= 0.03023;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M240(void)   {PWM1= 0.00000;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M241(void)   {PWM1=-0.03023;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M242(void)   {PWM1=-0.06048;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M243(void)   {PWM1=-0.09077;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M244(void)   {PWM1=-0.12112;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M245(void)   {PWM1=-0.15153;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M246(void)   {PWM1=-0.18205;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M247(void)   {PWM1=-0.21267;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M248(void)   {PWM1=-0.24342;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M249(void)   {PWM1=-0.27433;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M250(void)   {PWM1=-0.30541;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M251(void)   {PWM1=-0.33668;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M252(void)   {PWM1=-0.36816;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M253(void)   {PWM1=-0.39988;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M254(void)   {PWM1=-0.43185;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M255(void)   {PWM1=-0.46410;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M256(void)   {PWM1=-0.49666;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M257(void)   {PWM1=-0.52954;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M258(void)   {PWM1=-0.56278;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M259(void)   {PWM1=-0.59639;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M260(void)   {PWM1=-0.63041;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M261(void)   {PWM1=-0.66487;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M262(void)   {PWM1=-0.69979;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M263(void)   {PWM1=-0.73521;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M264(void)   {PWM1=-0.77116;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M265(void)   {PWM1=-0.80767;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M266(void)   {PWM1=-0.84478;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M267(void)   {PWM1=-0.88252;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M268(void)   {PWM1=-0.92095;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M269(void)   {PWM1=-0.96009;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M270(void)   {PWM1=-1.00000;   PWM2=-1.00000;   PWM3= 1.00000;}
        void M271(void)   {PWM1=-1.00000;   PWM2=-0.96009;   PWM3= 1.00000;}
        void M272(void)   {PWM1=-1.00000;   PWM2=-0.92095;   PWM3= 1.00000;}
        void M273(void)   {PWM1=-1.00000;   PWM2=-0.88252;   PWM3= 1.00000;}
        void M274(void)   {PWM1=-1.00000;   PWM2=-0.84478;   PWM3= 1.00000;}
        void M275(void)   {PWM1=-1.00000;   PWM2=-0.80767;   PWM3= 1.00000;}
        void M276(void)   {PWM1=-1.00000;   PWM2=-0.77116;   PWM3= 1.00000;}
        void M277(void)   {PWM1=-1.00000;   PWM2=-0.73521;   PWM3= 1.00000;}
        void M278(void)   {PWM1=-1.00000;   PWM2=-0.69979;   PWM3= 1.00000;}
        void M279(void)   {PWM1=-1.00000;   PWM2=-0.66487;   PWM3= 1.00000;}
        void M280(void)   {PWM1=-1.00000;   PWM2=-0.63041;   PWM3= 1.00000;}
        void M281(void)   {PWM1=-1.00000;   PWM2=-0.59639;   PWM3= 1.00000;}
        void M282(void)   {PWM1=-1.00000;   PWM2=-0.56278;   PWM3= 1.00000;}
        void M283(void)   {PWM1=-1.00000;   PWM2=-0.52954;   PWM3= 1.00000;}
        void M284(void)   {PWM1=-1.00000;   PWM2=-0.49666;   PWM3= 1.00000;}
        void M285(void)   {PWM1=-1.00000;   PWM2=-0.46410;   PWM3= 1.00000;}
        void M286(void)   {PWM1=-1.00000;   PWM2=-0.43185;   PWM3= 1.00000;}
        void M287(void)   {PWM1=-1.00000;   PWM2=-0.39988;   PWM3= 1.00000;}
        void M288(void)   {PWM1=-1.00000;   PWM2=-0.36816;   PWM3= 1.00000;}
        void M289(void)   {PWM1=-1.00000;   PWM2=-0.33668;   PWM3= 1.00000;}
        void M290(void)   {PWM1=-1.00000;   PWM2=-0.30541;   PWM3= 1.00000;}
        void M291(void)   {PWM1=-1.00000;   PWM2=-0.27433;   PWM3= 1.00000;}
        void M292(void)   {PWM1=-1.00000;   PWM2=-0.24342;   PWM3= 1.00000;}
        void M293(void)   {PWM1=-1.00000;   PWM2=-0.21267;   PWM3= 1.00000;}
        void M294(void)   {PWM1=-1.00000;   PWM2=-0.18205;   PWM3= 1.00000;}
        void M295(void)   {PWM1=-1.00000;   PWM2=-0.15153;   PWM3= 1.00000;}
        void M296(void)   {PWM1=-1.00000;   PWM2=-0.12112;   PWM3= 1.00000;}
        void M297(void)   {PWM1=-1.00000;   PWM2=-0.09077;   PWM3= 1.00000;}
        void M298(void)   {PWM1=-1.00000;   PWM2=-0.06048;   PWM3= 1.00000;}
        void M299(void)   {PWM1=-1.00000;   PWM2=-0.03023;   PWM3= 1.00000;}
        void M300(void)   {PWM1=-1.00000;   PWM2= 0.00000;   PWM3= 1.00000;}
        void M301(void)   {PWM1=-1.00000;   PWM2= 0.03023;   PWM3= 1.00000;}
        void M302(void)   {PWM1=-1.00000;   PWM2= 0.06048;   PWM3= 1.00000;}
        void M303(void)   {PWM1=-1.00000;   PWM2= 0.09077;   PWM3= 1.00000;}
        void M304(void)   {PWM1=-1.00000;   PWM2= 0.12112;   PWM3= 1.00000;}
        void M305(void)   {PWM1=-1.00000;   PWM2= 0.15153;   PWM3= 1.00000;}
        void M306(void)   {PWM1=-1.00000;   PWM2= 0.18205;   PWM3= 1.00000;}
        void M307(void)   {PWM1=-1.00000;   PWM2= 0.21267;   PWM3= 1.00000;}
        void M308(void)   {PWM1=-1.00000;   PWM2= 0.24342;   PWM3= 1.00000;}
        void M309(void)   {PWM1=-1.00000;   PWM2= 0.27433;   PWM3= 1.00000;}
        void M310(void)   {PWM1=-1.00000;   PWM2= 0.30541;   PWM3= 1.00000;}
        void M311(void)   {PWM1=-1.00000;   PWM2= 0.33668;   PWM3= 1.00000;}
        void M312(void)   {PWM1=-1.00000;   PWM2= 0.36816;   PWM3= 1.00000;}
        void M313(void)   {PWM1=-1.00000;   PWM2= 0.39988;   PWM3= 1.00000;}
        void M314(void)   {PWM1=-1.00000;   PWM2= 0.43185;   PWM3= 1.00000;}
        void M315(void)   {PWM1=-1.00000;   PWM2= 0.46410;   PWM3= 1.00000;}
        void M316(void)   {PWM1=-1.00000;   PWM2= 0.49666;   PWM3= 1.00000;}
        void M317(void)   {PWM1=-1.00000;   PWM2= 0.52954;   PWM3= 1.00000;}
        void M318(void)   {PWM1=-1.00000;   PWM2= 0.56278;   PWM3= 1.00000;}
        void M319(void)   {PWM1=-1.00000;   PWM2= 0.59639;   PWM3= 1.00000;}
        void M320(void)   {PWM1=-1.00000;   PWM2= 0.63041;   PWM3= 1.00000;}
        void M321(void)   {PWM1=-1.00000;   PWM2= 0.66487;   PWM3= 1.00000;}
        void M322(void)   {PWM1=-1.00000;   PWM2= 0.69979;   PWM3= 1.00000;}
        void M323(void)   {PWM1=-1.00000;   PWM2= 0.73521;   PWM3= 1.00000;}
        void M324(void)   {PWM1=-1.00000;   PWM2= 0.77116;   PWM3= 1.00000;}
        void M325(void)   {PWM1=-1.00000;   PWM2= 0.80767;   PWM3= 1.00000;}
        void M326(void)   {PWM1=-1.00000;   PWM2= 0.84478;   PWM3= 1.00000;}
        void M327(void)   {PWM1=-1.00000;   PWM2= 0.88252;   PWM3= 1.00000;}
        void M328(void)   {PWM1=-1.00000;   PWM2= 0.92095;   PWM3= 1.00000;}
        void M329(void)   {PWM1=-1.00000;   PWM2= 0.96009;   PWM3= 1.00000;}
        void M330(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 1.00000;}
        void M331(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.96009;}
        void M332(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.92095;}
        void M333(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.88252;}
        void M334(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.84478;}
        void M335(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.80767;}
        void M336(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.77116;}
        void M337(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.73521;}
        void M338(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.69979;}
        void M339(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.66487;}
        void M340(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.63041;}
        void M341(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.59639;}
        void M342(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.56278;}
        void M343(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.52954;}
        void M344(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.49666;}
        void M345(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.46410;}
        void M346(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.43185;}
        void M347(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.39988;}
        void M348(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.36816;}
        void M349(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.33668;}
        void M350(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.30541;}
        void M351(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.27433;}
        void M352(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.24342;}
        void M353(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.21267;}
        void M354(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.18205;}
        void M355(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.15153;}
        void M356(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.12112;}
        void M357(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.09077;}
        void M358(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.06048;}
        void M359(void)   {PWM1=-1.00000;   PWM2= 1.00000;   PWM3= 0.03023;}
    // モータのPWM制御分岐
    void (*PwmApi[])(void) = {M000, M001, M002, M003, M004, M005, M006, M007, M008, M009, M010, M011, M012, M013, M014, M015, M016, M017, M018, M019, M020, M021, M022, M023, M024, M025, M026, M027, M028, M029, M030, M031, M032, M033, M034, M035, M036, M037, M038, M039, M040, M041, M042, M043, M044, M045, M046, M047, M048, M049, M050, M051, M052, M053, M054, M055, M056, M057, M058, M059, M060, M061, M062, M063, M064, M065, M066, M067, M068, M069, M070, M071, M072, M073, M074, M075, M076, M077, M078, M079, M080, M081, M082, M083, M084, M085, M086, M087, M088, M089, M090, M091, M092, M093, M094, M095, M096, M097, M098, M099, M100, M101, M102, M103, M104, M105, M106, M107, M108, M109, M110, M111, M112, M113, M114, M115, M116, M117, M118, M119, M120, M121, M122, M123, M124, M125, M126, M127, M128, M129, M130, M131, M132, M133, M134, M135, M136, M137, M138, M139, M140, M141, M142, M143, M144, M145, M146, M147, M148, M149, M150, M151, M152, M153, M154, M155, M156, M157, M158, M159, M160, M161, M162, M163, M164, M165, M166, M167, M168, M169, M170, M171, M172, M173, M174, M175, M176, M177, M178, M179, M180, M181, M182, M183, M184, M185, M186, M187, M188, M189, M190, M191, M192, M193, M194, M195, M196, M197, M198, M199, M200, M201, M202, M203, M204, M205, M206, M207, M208, M209, M210, M211, M212, M213, M214, M215, M216, M217, M218, M219, M220, M221, M222, M223, M224, M225, M226, M227, M228, M229, M230, M231, M232, M233, M234, M235, M236, M237, M238, M239, M240, M241, M242, M243, M244, M245, M246, M247, M248, M249, M250, M251, M252, M253, M254, M255, M256, M257, M258, M259, M260, M261, M262, M263, M264, M265, M266, M267, M268, M269, M270, M271, M272, M273, M274, M275, M276, M277, M278, M279, M280, M281, M282, M283, M284, M285, M286, M287, M288, M289, M290, M291, M292, M293, M294, M295, M296, M297, M298, M299, M300, M301, M302, M303, M304, M305, M306, M307, M308, M309, M310, M311, M312, M313, M314, M315, M316, M317, M318, M319, M320, M321, M322, M323, M324, M325, M326, M327, M328, M329, M330, M331, M332, M333, M334, M335, M336, M337, M338, M339, M340, M341, M342, M343, M344, M345, M346, M347, M348, M349, M350, M351, M352, M353, M354, M355, M356, M357, M358, M359};


    void Moter(float speed, int angle, float omega) {
        (*PwmApi[angle])();
        PWM1 += omega;  PWM2 += omega;  PWM3 += omega;
        MoterApi(((PWM1>=0)?1:0), ((PWM2>=0)?1:0), ((PWM3>=0)?1:0));

        PWM1 = abs(PWM1);    PWM2 = abs(PWM2);    PWM3 = abs(PWM3);
        // 最大値を探す
        float max = (PWM1 > PWM2) ? PWM1 : PWM2;
              max = (max  > PWM3) ? max  : PWM3;

        PWM1 = PWM1/max*speed;    PWM2 = PWM2/max*speed;    PWM3 = PWM3/max*speed;
        if(max == 0){PWM1=0; PWM2=0; PWM3=0;}
        pwm1 = PWM1/2;    pwm2 = PWM2/2;    pwm3 = PWM3/2;
        led4 = 1;
    }


    // 角度自動修正関数
    float Auto_Corrction(void) {
        float omega = 0.00000;
        if (45.0 < Compass && Compass < 180.0) {
            omega = -0.3;
        } else if (-180.0 < Compass && Compass < -45.0) {
            omega =  0.3;
        } else {
            omega =  0.000000;
        }
        return omega;
    }


    // モータの回転方向を制御
    void MoterApi(bool m1, bool m2, bool m3) {
        M1_1 = m1;   M1_2 = !m1;
        M2_1 = m2;   M2_2 = !m2;
        M3_1 = m3;   M3_2 = !m3;
    }

    // モータをすべて止める
    void MoterReset (bool m1, bool m2, bool m3) {
        if (!m1) { M1_1 = 0;   M1_2 = 0; }
        if (!m2) { M2_1 = 0;   M2_2 = 0; }
        if (!m3) { M3_1 = 0;   M3_2 = 0; }
    }


















