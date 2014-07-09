// Slave側プログラム

#include <math.h>
#include "mbed.h"
#include "rtos.h"
#include "ACM1602NI.h"
#include "Ping.h"

#define PI 3.14159265358979 // 円周率πの定義

Semaphore PING_slots(2);




// ピンの設定
    DigitalOut led1(LED1);
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);
    DigitalOut led4(LED4);

    // ロータリースイッチ
    DigitalIn Rotary1(p5);
    DigitalIn Rotary2(p6);
    DigitalIn Rotary3(p7);
    DigitalIn Rotary4(p8);

    // ボール保持確認ピン
    DigitalIn BallCheck(p29);

    // それぞれのマイコンとの通信
    I2CSlave I2C_avr(p28, p27);
    I2CSlave I2C_master(p9, p10);

    /* 超音波距離センサのピンを設定 */
    Ping PING_F(p24);
    Ping PING_R(p21);
    Ping PING_B(p22);
    Ping PING_L(p23);




// プロトタイプ宣言
    /*---- RotarySW.h ----*/
        void FW(void);
        void DF(void);
        void DEBUG_IR(void);
        void DEBUG_PING(void);
        void DEBUG_ANGLE(void);
        void DEBUG_KICKER(void);
        void DEBUG_MOTER(void);
        void DEBUG_LINE(void);


    /*---- I2C_Slave.h ----*/
        void MBED_PING(void);
        void MBED_IR(void);
        void MBED_LCD(void);
        void MBED_ROTARY(void);
        void MBED_MASTER(void);
        void int_char(int x);



// グローバル変数
    int val = 0;

    char Mode[1] = {0}; // ロータリースイッチの状態

    char lcd[18];       // LCD送信用データ変数

    char Ping_data[4];  // PINGの送信用データ変数

    char IR_data[4];    // IR受信用データ変数
    char Angle_char[] = {0, 0, 100, 0}; // {進行方向１, 進行方向２, スピード, キッカー}

    char MBED_data[1];      // mbed_masterからの受信用データ変数
    int  Order = 0;         // 命令受信用

    bool Flag = 0;





/*---- PINGの値を並列処理で取得し続ける ----*/
void PING_thread(void const *ping_data) {
    /* 距離記録用変数 */
    int Ping_F = 0;
    int Ping_R = 0;
    int Ping_B = 0;
    int Ping_L = 0;

    while(1) {
        /* 超音波発射 */
        PING_F.Send();
        PING_R.Send();
        PING_B.Send();
        PING_L.Send();
        wait_ms(30);
        /* 結果から距離を算出 */
        Ping_F = PING_F.Read_cm() / 2;  // 正面の距離を記録 [cm]
        Ping_R = PING_R.Read_cm() / 2;  // 右側の距離を記録 [cm]
        Ping_B = PING_B.Read_cm() / 2;  // 後ろの距離を記録 [cm]
        Ping_L = PING_L.Read_cm() / 2;  // 左側の距離を記録 [cm]

        /* アクセス可能になるまで待機してから、値を代入 */
        PING_slots.wait();
            Ping_data[0] = Ping_F;
            Ping_data[1] = Ping_R;
            Ping_data[2] = Ping_B;
            Ping_data[3] = Ping_L;
        PING_slots.release();

        wait(0.1);      // PINGに愛を！！
    }
}



/*---- 並列処理で各装置との通信を制御 ----*/
int main() {
    Thread PING_t(PING_thread);

    /*---- 初期化処理 ----*/
    I2C_avr.address(0x80);
    I2C_master.address(0xA0);

    /*---- modeを確認 ----*/
    Mode[0] = ((Rotary1)?0:1)*1 + ((Rotary2)?0:1)*2 + ((Rotary3)?0:1)*4 + ((Rotary4)?0:1)*8;

    /*---- mode_masterにmodeを伝える ---*/
    while (!Flag) MBED_MASTER();

    /*---- 各モードに移行 ----*/
    switch(Mode[0]) {
        case 0: while(1) FW();
        case 1: while(1) DF();
        case 2: while(1) DEBUG_IR();
        case 3: while(1) DEBUG_PING();
        case 4: while(1) DEBUG_ANGLE();
        case 5: while(1) DEBUG_KICKER();
        case 6: while(1) DEBUG_MOTER();
        case 7: while(1) DEBUG_LINE();
        case 8: while(1) FW();
        case 9: while(1) FW();
    }
}




// ロータリースイッチの設定
    // "0"攻撃モード
    void FW(void) {
        MBED_MASTER();
    }

    // "1"防御モード
    void DF(void) {
        MBED_MASTER();
        int count = 0;
        for(int i=0; i<8; i++) {
            count += (IR_data[1] % 2)?1:0;
            IR_data[1] = 2;
        }
        Angle_char[3] = 100.0 / 8.0 * count;
    }

    // "2"IRデバッグモード
    void DEBUG_IR(void) {
        int debug_mode = I2C_master.receive();
        int j = I2C_avr.receive();
        switch(debug_mode) {
            case I2CSlave::ReadAddressed:   // データ送信要求
                do val = I2C_master.write(IR_data, 2); while(val);
                break;
            // 基本的に使わない処理
                case I2CSlave::WriteAddressed:  // データ受信
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
        switch (j) {
            case I2CSlave::WriteAddressed:
                val = I2C_avr.read(IR_data, 2);     // 0:near  1:long
                if(!val) led4=1;
                break;
            // 基本的に使わない処理
                case I2CSlave::ReadAddressed:
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }


    // "3"PINGデバッグモード
    void DEBUG_PING(void) {
        int debug_mode = I2C_master.receive();
        switch(debug_mode) {
            case I2CSlave::ReadAddressed:   // データ送信要求
                PING_slots.wait();
                    val = I2C_master.write(Ping_data, 4);
                PING_slots.release();
                break;
            // 基本的に使わない処理
                case I2CSlave::WriteAddressed:  // データ受信
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }

    // "4"ANGLEデバッグモード
    void DEBUG_ANGLE(void) {
        int debug_mode = I2C_master.receive();
        switch(debug_mode) {
            // 基本的に使わない処理
                case I2CSlave::ReadAddressed:   // データ送信要求
                    break;
                case I2CSlave::WriteAddressed:  // データ受信
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }

    // "5"KICKERデバッグモード
    void DEBUG_KICKER(void) {
        int debug_mode = I2C_master.receive();
        switch(debug_mode) {
            // 基本的に使わない処理
                case I2CSlave::ReadAddressed:   // データ送信要求
                    break;
                case I2CSlave::WriteAddressed:  // データ受信
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }

    // "6"MOTERデバッグモード
    void DEBUG_MOTER(void) {
        int debug_mode = I2C_master.receive();
        switch(debug_mode) {
            // 基本的に使わない処理
                case I2CSlave::ReadAddressed:   // データ送信要求
                    break;
                case I2CSlave::WriteAddressed:  // データ受信
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }

    // "7"LINEデバッグモード
    void DEBUG_LINE(void) {
        int debug_mode = I2C_master.receive();
        switch(debug_mode) {
            // 基本的に使わない処理
                case I2CSlave::ReadAddressed:   // データ送信要求
                    break;
                case I2CSlave::WriteAddressed:  // データ受信
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }



    void (*Debug_Mode[])(void) = {FW, DF, DEBUG_IR, DEBUG_PING, DEBUG_ANGLE, DEBUG_KICKER, DEBUG_MOTER, DEBUG_LINE};




// I2Cの設定
    /*---- FWの進行方向の処理 -----*/
    // IRのデータによるAngleの処理
        int FW_000(void)    {return 001;}
        int FW_001(void)    {return 000;}
        int FW_002(void)    {return  90;}
        int FW_003(void)    {return 045;}
        int FW_004(void)    {return 135;}
        int FW_005(void)    {return  90;}
        int FW_006(void)    {return 113;}
        int FW_007(void)    {return  90;}
        int FW_008(void)    {return 180;}
        int FW_009(void)    {return 001;}
        int FW_010(void)    {return 135;}
        int FW_011(void)    {return 001;}
        int FW_012(void)    {return 158;}
        int FW_013(void)    {return 001;}
        int FW_014(void)    {return 135;}
        int FW_015(void)    {return 113;}
        int FW_016(void)    {return  90;}
        int FW_017(void)    {return 001;}
        int FW_018(void)    {return 001;}
        int FW_019(void)    {return 001;}
        int FW_020(void)    {return 001;}
        int FW_021(void)    {return 001;}
        int FW_022(void)    {return 001;}
        int FW_023(void)    {return 001;}
        int FW_024(void)    {return 225;}
        int FW_025(void)    {return 001;}
        int FW_026(void)    {return 001;}
        int FW_027(void)    {return 001;}
        int FW_028(void)    {return 195;}
        int FW_029(void)    {return 001;}
        int FW_030(void)    {return 203;}
        int FW_031(void)    {return 180;}
        int FW_032(void)    {return 180;}
        int FW_033(void)    {return 001;}
        int FW_034(void)    {return 001;}
        int FW_035(void)    {return 001;}
        int FW_036(void)    {return 001;}
        int FW_037(void)    {return 001;}
        int FW_038(void)    {return 001;}
        int FW_039(void)    {return 001;}
        int FW_040(void)    {return 001;}
        int FW_041(void)    {return 001;}
        int FW_042(void)    {return 001;}
        int FW_043(void)    {return 001;}
        int FW_044(void)    {return 001;}
        int FW_045(void)    {return 001;}
        int FW_046(void)    {return 001;}
        int FW_047(void)    {return 001;}
        int FW_048(void)    {return 135;}
        int FW_049(void)    {return 001;}
        int FW_050(void)    {return 001;}
        int FW_051(void)    {return 001;}
        int FW_052(void)    {return 001;}
        int FW_053(void)    {return 001;}
        int FW_054(void)    {return 001;}
        int FW_055(void)    {return 001;}
        int FW_056(void)    {return  90;}
        int FW_057(void)    {return 001;}
        int FW_058(void)    {return 001;}
        int FW_059(void)    {return 001;}
        int FW_060(void)    {return 248;}
        int FW_061(void)    {return 001;}
        int FW_062(void)    {return 225;}
        int FW_063(void)    {return 203;}
        int FW_064(void)    {return 225;}
        int FW_065(void)    {return 001;}
        int FW_066(void)    {return 001;}
        int FW_067(void)    {return 001;}
        int FW_068(void)    {return 001;}
        int FW_069(void)    {return 001;}
        int FW_070(void)    {return 001;}
        int FW_071(void)    {return 001;}
        int FW_072(void)    {return 001;}
        int FW_073(void)    {return 001;}
        int FW_074(void)    {return 001;}
        int FW_075(void)    {return 001;}
        int FW_076(void)    {return 001;}
        int FW_077(void)    {return 001;}
        int FW_078(void)    {return 001;}
        int FW_079(void)    {return 001;}
        int FW_080(void)    {return 001;}
        int FW_081(void)    {return 001;}
        int FW_082(void)    {return 001;}
        int FW_083(void)    {return 001;}
        int FW_084(void)    {return 001;}
        int FW_085(void)    {return 001;}
        int FW_086(void)    {return 001;}
        int FW_087(void)    {return 001;}
        int FW_088(void)    {return 001;}
        int FW_089(void)    {return 001;}
        int FW_090(void)    {return 001;}
        int FW_091(void)    {return 001;}
        int FW_092(void)    {return 001;}
        int FW_093(void)    {return 001;}
        int FW_094(void)    {return 001;}
        int FW_095(void)    {return 001;}
        int FW_096(void)    {return 203;}
        int FW_097(void)    {return 001;}
        int FW_098(void)    {return 001;}
        int FW_099(void)    {return 001;}
        int FW_100(void)    {return 001;}
        int FW_101(void)    {return 001;}
        int FW_102(void)    {return 001;}
        int FW_103(void)    {return 001;}
        int FW_104(void)    {return 001;}
        int FW_105(void)    {return 001;}
        int FW_106(void)    {return 001;}
        int FW_107(void)    {return 001;}
        int FW_108(void)    {return 001;}
        int FW_109(void)    {return 001;}
        int FW_110(void)    {return 001;}
        int FW_111(void)    {return 001;}
        int FW_112(void)    {return 135;}
        int FW_113(void)    {return 001;}
        int FW_114(void)    {return 001;}
        int FW_115(void)    {return 001;}
        int FW_116(void)    {return 001;}
        int FW_117(void)    {return 001;}
        int FW_118(void)    {return 001;}
        int FW_119(void)    {return 001;}
        int FW_120(void)    {return 112;}
        int FW_121(void)    {return 001;}
        int FW_122(void)    {return 001;}
        int FW_123(void)    {return 001;}
        int FW_124(void)    {return  90;}
        int FW_125(void)    {return 001;}
        int FW_126(void)    {return 248;}
        int FW_127(void)    {return 225;}
        int FW_128(void)    {return 270;}
        int FW_129(void)    {return 310;}
        int FW_130(void)    {return 001;}
        int FW_131(void)    {return 000;}
        int FW_132(void)    {return 001;}
        int FW_133(void)    {return 001;}
        int FW_134(void)    {return 001;}
        int FW_135(void)    {return 045;}
        int FW_136(void)    {return 001;}
        int FW_137(void)    {return 001;}
        int FW_138(void)    {return 001;}
        int FW_139(void)    {return 001;}
        int FW_140(void)    {return 001;}
        int FW_141(void)    {return 001;}
        int FW_142(void)    {return 001;}
        int FW_143(void)    {return 135;}
        int FW_144(void)    {return 001;}
        int FW_145(void)    {return 001;}
        int FW_146(void)    {return 001;}
        int FW_147(void)    {return 001;}
        int FW_148(void)    {return 001;}
        int FW_149(void)    {return 001;}
        int FW_150(void)    {return 001;}
        int FW_151(void)    {return 001;}
        int FW_152(void)    {return 001;}
        int FW_153(void)    {return 001;}
        int FW_154(void)    {return 001;}
        int FW_155(void)    {return 001;}
        int FW_156(void)    {return 001;}
        int FW_157(void)    {return 001;}
        int FW_158(void)    {return 001;}
        int FW_159(void)    {return 158;}
        int FW_160(void)    {return 001;}
        int FW_161(void)    {return 001;}
        int FW_162(void)    {return 001;}
        int FW_163(void)    {return 001;}
        int FW_164(void)    {return 001;}
        int FW_165(void)    {return 001;}
        int FW_166(void)    {return 001;}
        int FW_167(void)    {return 001;}
        int FW_168(void)    {return 001;}
        int FW_169(void)    {return 001;}
        int FW_170(void)    {return 001;}
        int FW_171(void)    {return 001;}
        int FW_172(void)    {return 001;}
        int FW_173(void)    {return 001;}
        int FW_174(void)    {return 001;}
        int FW_175(void)    {return 001;}
        int FW_176(void)    {return 001;}
        int FW_177(void)    {return 001;}
        int FW_178(void)    {return 001;}
        int FW_179(void)    {return 001;}
        int FW_180(void)    {return 001;}
        int FW_181(void)    {return 001;}
        int FW_182(void)    {return 001;}
        int FW_183(void)    {return 001;}
        int FW_184(void)    {return 001;}
        int FW_185(void)    {return 001;}
        int FW_186(void)    {return 001;}
        int FW_187(void)    {return 001;}
        int FW_188(void)    {return 001;}
        int FW_189(void)    {return 001;}
        int FW_190(void)    {return 001;}
        int FW_191(void)    {return 180;}
        int FW_192(void)    {return 247;}
        int FW_193(void)    {return 001;}
        int FW_194(void)    {return 001;}
        int FW_195(void)    {return 001;}
        int FW_196(void)    {return 001;}
        int FW_197(void)    {return 001;}
        int FW_198(void)    {return 001;}
        int FW_199(void)    {return 000;}
        int FW_200(void)    {return 001;}
        int FW_201(void)    {return 001;}
        int FW_202(void)    {return 001;}
        int FW_203(void)    {return 001;}
        int FW_204(void)    {return 001;}
        int FW_205(void)    {return 001;}
        int FW_206(void)    {return 001;}
        int FW_207(void)    {return 001;}
        int FW_208(void)    {return 001;}
        int FW_209(void)    {return 001;}
        int FW_210(void)    {return 001;}
        int FW_211(void)    {return 001;}
        int FW_212(void)    {return 001;}
        int FW_213(void)    {return 001;}
        int FW_214(void)    {return 001;}
        int FW_215(void)    {return 001;}
        int FW_216(void)    {return 001;}
        int FW_217(void)    {return 001;}
        int FW_218(void)    {return 001;}
        int FW_219(void)    {return 001;}
        int FW_220(void)    {return 001;}
        int FW_221(void)    {return 001;}
        int FW_222(void)    {return 001;}
        int FW_223(void)    {return 001;}
        int FW_224(void)    {return 180;}
        int FW_225(void)    {return 001;}
        int FW_226(void)    {return 001;}
        int FW_227(void)    {return 001;}
        int FW_228(void)    {return 001;}
        int FW_229(void)    {return 001;}
        int FW_230(void)    {return 001;}
        int FW_231(void)    {return 001;}
        int FW_232(void)    {return 001;}
        int FW_233(void)    {return 001;}
        int FW_234(void)    {return 001;}
        int FW_235(void)    {return 001;}
        int FW_236(void)    {return 001;}
        int FW_237(void)    {return 001;}
        int FW_238(void)    {return 001;}
        int FW_239(void)    {return 000;}
        int FW_240(void)    {return 158;}
        int FW_241(void)    {return 001;}
        int FW_242(void)    {return 001;}
        int FW_243(void)    {return 001;}
        int FW_244(void)    {return 001;}
        int FW_245(void)    {return 001;}
        int FW_246(void)    {return 001;}
        int FW_247(void)    {return 001;}
        int FW_248(void)    {return 135;}
        int FW_249(void)    {return 001;}
        int FW_250(void)    {return 001;}
        int FW_251(void)    {return 001;}
        int FW_252(void)    {return 202;}
        int FW_253(void)    {return 001;}
        int FW_254(void)    {return  90;}
        int FW_255(void)    {return 001;}
    // IRのデータからAngleの分岐
    int (*FW_AngApi[])(void) = {FW_001, FW_001, FW_002, FW_003, FW_004, FW_005, FW_006, FW_007, FW_008, FW_009, FW_010, FW_011, FW_012, FW_013, FW_014, FW_015, FW_016, FW_017, FW_018, FW_019, FW_020, FW_021, FW_022, FW_023, FW_024, FW_025, FW_026, FW_027, FW_028, FW_029, FW_030, FW_031, FW_032, FW_033, FW_034, FW_035, FW_036, FW_037, FW_038, FW_039, FW_040, FW_041, FW_042, FW_043, FW_044, FW_045, FW_046, FW_047, FW_048, FW_049, FW_050, FW_051, FW_052, FW_053, FW_054, FW_055, FW_056, FW_057, FW_058, FW_059, FW_060, FW_061, FW_062, FW_063, FW_064, FW_065, FW_066, FW_067, FW_068, FW_069, FW_070, FW_071, FW_072, FW_073, FW_074, FW_075, FW_076, FW_077, FW_078, FW_079, FW_080, FW_081, FW_082, FW_083, FW_084, FW_085, FW_086, FW_087, FW_088, FW_089, FW_090, FW_091, FW_092, FW_093, FW_094, FW_095, FW_096, FW_097, FW_098, FW_099, FW_100, FW_101, FW_102, FW_103, FW_104, FW_105, FW_106, FW_107, FW_108, FW_109, FW_110, FW_111, FW_112, FW_113, FW_114, FW_115, FW_116, FW_117, FW_118, FW_119, FW_120, FW_121, FW_122, FW_123, FW_124, FW_125, FW_126, FW_127, FW_128, FW_129, FW_130, FW_131, FW_132, FW_133, FW_134, FW_135, FW_136, FW_137, FW_138, FW_139, FW_140, FW_141, FW_142, FW_143, FW_144, FW_145, FW_146, FW_147, FW_148, FW_149, FW_150, FW_151, FW_152, FW_153, FW_154, FW_155, FW_156, FW_157, FW_158, FW_159, FW_160, FW_161, FW_162, FW_163, FW_164, FW_165, FW_166, FW_167, FW_168, FW_169, FW_170, FW_171, FW_172, FW_173, FW_174, FW_175, FW_176, FW_177, FW_178, FW_179, FW_180, FW_181, FW_182, FW_183, FW_184, FW_185, FW_186, FW_187, FW_188, FW_189, FW_190, FW_191, FW_192, FW_193, FW_194, FW_195, FW_196, FW_197, FW_198, FW_199, FW_200, FW_201, FW_202, FW_203, FW_204, FW_205, FW_206, FW_207, FW_208, FW_209, FW_210, FW_211, FW_212, FW_213, FW_214, FW_215, FW_216, FW_217, FW_218, FW_219, FW_220, FW_221, FW_222, FW_223, FW_224, FW_225, FW_226, FW_227, FW_228, FW_229, FW_230, FW_231, FW_232, FW_233, FW_234, FW_235, FW_236, FW_237, FW_238, FW_239, FW_240, FW_241, FW_242, FW_243, FW_244, FW_245, FW_246, FW_247, FW_248, FW_249, FW_250, FW_251, FW_252, FW_253, FW_254, FW_255};


    /*---- DFの進行方向の処理 -----*/
    // IRのデータによるAngleの処理
        int DF_000(void)    {return 001;}
        int DF_001(void)    {return 001;}
        int DF_002(void)    {return  90;}
        int DF_003(void)    {return 023;}
        int DF_004(void)    {return  90;}
        int DF_005(void)    {return 001;}
        int DF_006(void)    {return  90;}
        int DF_007(void)    {return 045;}
        int DF_008(void)    {return 180;}
        int DF_009(void)    {return 001;}
        int DF_010(void)    {return 001;}
        int DF_011(void)    {return 001;}
        int DF_012(void)    {return 180;}
        int DF_013(void)    {return 001;}
        int DF_014(void)    {return 135;}
        int DF_015(void)    {return  90;}
        int DF_016(void)    {return  90;}
        int DF_017(void)    {return 001;}
        int DF_018(void)    {return 001;}
        int DF_019(void)    {return 001;}
        int DF_020(void)    {return 001;}
        int DF_021(void)    {return 001;}
        int DF_022(void)    {return 001;}
        int DF_023(void)    {return 001;}
        int DF_024(void)    {return 203;}
        int DF_025(void)    {return 001;}
        int DF_026(void)    {return 001;}
        int DF_027(void)    {return 001;}
        int DF_028(void)    {return 180;}
        int DF_029(void)    {return 001;}
        int DF_030(void)    {return 157;}
        int DF_031(void)    {return  90;}
        int DF_032(void)    {return 180;}
        int DF_033(void)    {return 001;}
        int DF_034(void)    {return 001;}
        int DF_035(void)    {return 001;}
        int DF_036(void)    {return 001;}
        int DF_037(void)    {return 001;}
        int DF_038(void)    {return 001;}
        int DF_039(void)    {return 001;}
        int DF_040(void)    {return 001;}
        int DF_041(void)    {return 001;}
        int DF_042(void)    {return 001;}
        int DF_043(void)    {return 001;}
        int DF_044(void)    {return 001;}
        int DF_045(void)    {return 001;}
        int DF_046(void)    {return 001;}
        int DF_047(void)    {return 001;}
        int DF_048(void)    {return 157;}
        int DF_049(void)    {return 001;}
        int DF_050(void)    {return 001;}
        int DF_051(void)    {return 001;}
        int DF_052(void)    {return 001;}
        int DF_053(void)    {return 001;}
        int DF_054(void)    {return 001;}
        int DF_055(void)    {return 001;}
        int DF_056(void)    {return  90;}
        int DF_057(void)    {return 001;}
        int DF_058(void)    {return 001;}
        int DF_059(void)    {return 001;}
        int DF_060(void)    {return 225;}
        int DF_061(void)    {return 001;}
        int DF_062(void)    {return 180;}
        int DF_063(void)    {return 180;}
        int DF_064(void)    {return 180;}
        int DF_065(void)    {return 001;}
        int DF_066(void)    {return 001;}
        int DF_067(void)    {return 001;}
        int DF_068(void)    {return 001;}
        int DF_069(void)    {return 001;}
        int DF_070(void)    {return 001;}
        int DF_071(void)    {return 001;}
        int DF_072(void)    {return 001;}
        int DF_073(void)    {return 001;}
        int DF_074(void)    {return 001;}
        int DF_075(void)    {return 001;}
        int DF_076(void)    {return 001;}
        int DF_077(void)    {return 001;}
        int DF_078(void)    {return 001;}
        int DF_079(void)    {return 001;}
        int DF_080(void)    {return 001;}
        int DF_081(void)    {return 001;}
        int DF_082(void)    {return 001;}
        int DF_083(void)    {return 001;}
        int DF_084(void)    {return 001;}
        int DF_085(void)    {return 001;}
        int DF_086(void)    {return 001;}
        int DF_087(void)    {return 001;}
        int DF_088(void)    {return 001;}
        int DF_089(void)    {return 001;}
        int DF_090(void)    {return 001;}
        int DF_091(void)    {return 001;}
        int DF_092(void)    {return 001;}
        int DF_093(void)    {return 001;}
        int DF_094(void)    {return 001;}
        int DF_095(void)    {return 001;}
        int DF_096(void)    {return 180;}
        int DF_097(void)    {return 001;}
        int DF_098(void)    {return 001;}
        int DF_099(void)    {return 001;}
        int DF_100(void)    {return 001;}
        int DF_101(void)    {return 001;}
        int DF_102(void)    {return 001;}
        int DF_103(void)    {return 001;}
        int DF_104(void)    {return 001;}
        int DF_105(void)    {return 001;}
        int DF_106(void)    {return 001;}
        int DF_107(void)    {return 001;}
        int DF_108(void)    {return 001;}
        int DF_109(void)    {return 001;}
        int DF_110(void)    {return 001;}
        int DF_111(void)    {return 001;}
        int DF_112(void)    {return 135;}
        int DF_113(void)    {return 001;}
        int DF_114(void)    {return 001;}
        int DF_115(void)    {return 001;}
        int DF_116(void)    {return 001;}
        int DF_117(void)    {return 001;}
        int DF_118(void)    {return 001;}
        int DF_119(void)    {return 001;}
        int DF_120(void)    {return 067;}
        int DF_121(void)    {return 001;}
        int DF_122(void)    {return 001;}
        int DF_123(void)    {return 001;}
        int DF_124(void)    {return  90;}
        int DF_125(void)    {return 001;}
        int DF_126(void)    {return 203;}
        int DF_127(void)    {return 180;}
        int DF_128(void)    {return 270;}
        int DF_129(void)    {return 315;}
        int DF_130(void)    {return 001;}
        int DF_131(void)    {return 000;}
        int DF_132(void)    {return 001;}
        int DF_133(void)    {return 001;}
        int DF_134(void)    {return 001;}
        int DF_135(void)    {return 023;}
        int DF_136(void)    {return 001;}
        int DF_137(void)    {return 001;}
        int DF_138(void)    {return 001;}
        int DF_139(void)    {return 001;}
        int DF_140(void)    {return 001;}
        int DF_141(void)    {return 001;}
        int DF_142(void)    {return 001;}
        int DF_143(void)    {return 045;}
        int DF_144(void)    {return 001;}
        int DF_145(void)    {return 001;}
        int DF_146(void)    {return 001;}
        int DF_147(void)    {return 001;}
        int DF_148(void)    {return 001;}
        int DF_149(void)    {return 001;}
        int DF_150(void)    {return 001;}
        int DF_151(void)    {return 001;}
        int DF_152(void)    {return 001;}
        int DF_153(void)    {return 001;}
        int DF_154(void)    {return 001;}
        int DF_155(void)    {return 001;}
        int DF_156(void)    {return 001;}
        int DF_157(void)    {return 001;}
        int DF_158(void)    {return 001;}
        int DF_159(void)    {return 067;}
        int DF_160(void)    {return 001;}
        int DF_161(void)    {return 001;}
        int DF_162(void)    {return 001;}
        int DF_163(void)    {return 001;}
        int DF_164(void)    {return 001;}
        int DF_165(void)    {return 001;}
        int DF_166(void)    {return 001;}
        int DF_167(void)    {return 001;}
        int DF_168(void)    {return 001;}
        int DF_169(void)    {return 001;}
        int DF_170(void)    {return 001;}
        int DF_171(void)    {return 001;}
        int DF_172(void)    {return 001;}
        int DF_173(void)    {return 001;}
        int DF_174(void)    {return 001;}
        int DF_175(void)    {return 001;}
        int DF_176(void)    {return 001;}
        int DF_177(void)    {return 001;}
        int DF_178(void)    {return 001;}
        int DF_179(void)    {return 001;}
        int DF_180(void)    {return 001;}
        int DF_181(void)    {return 001;}
        int DF_182(void)    {return 001;}
        int DF_183(void)    {return 001;}
        int DF_184(void)    {return 001;}
        int DF_185(void)    {return 001;}
        int DF_186(void)    {return 001;}
        int DF_187(void)    {return 001;}
        int DF_188(void)    {return 001;}
        int DF_189(void)    {return 001;}
        int DF_190(void)    {return 001;}
        int DF_191(void)    {return  90;}
        int DF_192(void)    {return 270;}
        int DF_193(void)    {return 315;}
        int DF_194(void)    {return 001;}
        int DF_195(void)    {return 337;}
        int DF_196(void)    {return 001;}
        int DF_197(void)    {return 001;}
        int DF_198(void)    {return 001;}
        int DF_199(void)    {return 000;}
        int DF_200(void)    {return 001;}
        int DF_201(void)    {return 001;}
        int DF_202(void)    {return 001;}
        int DF_203(void)    {return 001;}
        int DF_204(void)    {return 001;}
        int DF_205(void)    {return 001;}
        int DF_206(void)    {return 001;}
        int DF_207(void)    {return 045;}
        int DF_208(void)    {return 001;}
        int DF_209(void)    {return 001;}
        int DF_210(void)    {return 001;}
        int DF_211(void)    {return 001;}
        int DF_212(void)    {return 001;}
        int DF_213(void)    {return 001;}
        int DF_214(void)    {return 001;}
        int DF_215(void)    {return 001;}
        int DF_216(void)    {return 001;}
        int DF_217(void)    {return 001;}
        int DF_218(void)    {return 001;}
        int DF_219(void)    {return 001;}
        int DF_220(void)    {return 001;}
        int DF_221(void)    {return 001;}
        int DF_222(void)    {return 001;}
        int DF_223(void)    {return 001;}
        int DF_224(void)    {return 225;}
        int DF_225(void)    {return 270;}
        int DF_226(void)    {return 001;}
        int DF_227(void)    {return 315;}
        int DF_228(void)    {return 001;}
        int DF_229(void)    {return 001;}
        int DF_230(void)    {return 001;}
        int DF_231(void)    {return 337;}
        int DF_232(void)    {return 001;}
        int DF_233(void)    {return 001;}
        int DF_234(void)    {return 001;}
        int DF_235(void)    {return 001;}
        int DF_236(void)    {return 001;}
        int DF_237(void)    {return 001;}
        int DF_238(void)    {return 001;}
        int DF_239(void)    {return 000;}
        int DF_240(void)    {return 180;}
        int DF_241(void)    {return 001;}
        int DF_242(void)    {return 001;}
        int DF_243(void)    {return 001;}
        int DF_244(void)    {return 001;}
        int DF_245(void)    {return 001;}
        int DF_246(void)    {return 001;}
        int DF_247(void)    {return 001;}
        int DF_248(void)    {return 135;}
        int DF_249(void)    {return 001;}
        int DF_250(void)    {return 001;}
        int DF_251(void)    {return 001;}
        int DF_252(void)    {return 113;}
        int DF_253(void)    {return 001;}
        int DF_254(void)    {return  90;}
        int DF_255(void)    {return 001;}
    // IRのデータからAngleの分岐
    int (*DF_AngApi[])(void) = {DF_000, DF_001, DF_002, DF_003, DF_004, DF_005, DF_006, DF_007, DF_008, DF_009, DF_010, DF_011, DF_012, DF_013, DF_014, DF_015, DF_016, DF_017, DF_018, DF_019, DF_020, DF_021, DF_022, DF_023, DF_024, DF_025, DF_026, DF_027, DF_028, DF_029, DF_030, DF_031, DF_032, DF_033, DF_034, DF_035, DF_036, DF_037, DF_038, DF_039, DF_040, DF_041, DF_042, DF_043, DF_044, DF_045, DF_046, DF_047, DF_048, DF_049, DF_050, DF_051, DF_052, DF_053, DF_054, DF_055, DF_056, DF_057, DF_058, DF_059, DF_060, DF_061, DF_062, DF_063, DF_064, DF_065, DF_066, DF_067, DF_068, DF_069, DF_070, DF_071, DF_072, DF_073, DF_074, DF_075, DF_076, DF_077, DF_078, DF_079, DF_080, DF_081, DF_082, DF_083, DF_084, DF_085, DF_086, DF_087, DF_088, DF_089, DF_090, DF_091, DF_092, DF_093, DF_094, DF_095, DF_096, DF_097, DF_098, DF_099, DF_100, DF_101, DF_102, DF_103, DF_104, DF_105, DF_106, DF_107, DF_108, DF_109, DF_110, DF_111, DF_112, DF_113, DF_114, DF_115, DF_116, DF_117, DF_118, DF_119, DF_120, DF_121, DF_122, DF_123, DF_124, DF_125, DF_126, DF_127, DF_128, DF_129, DF_130, DF_131, DF_132, DF_133, DF_134, DF_135, DF_136, DF_137, DF_138, DF_139, DF_140, DF_141, DF_142, DF_143, DF_144, DF_145, DF_146, DF_147, DF_148, DF_149, DF_150, DF_151, DF_152, DF_153, DF_154, DF_155, DF_156, DF_157, DF_158, DF_159, DF_160, DF_161, DF_162, DF_163, DF_164, DF_165, DF_166, DF_167, DF_168, DF_169, DF_170, DF_171, DF_172, DF_173, DF_174, DF_175, DF_176, DF_177, DF_178, DF_179, DF_180, DF_181, DF_182, DF_183, DF_184, DF_185, DF_186, DF_187, DF_188, DF_189, DF_190, DF_191, DF_192, DF_193, DF_194, DF_195, DF_196, DF_197, DF_198, DF_199, DF_200, DF_201, DF_202, DF_203, DF_204, DF_205, DF_206, DF_207, DF_208, DF_209, DF_210, DF_211, DF_212, DF_213, DF_214, DF_215, DF_216, DF_217, DF_218, DF_219, DF_220, DF_221, DF_222, DF_223, DF_224, DF_225, DF_226, DF_227, DF_228, DF_229, DF_230, DF_231, DF_232, DF_233, DF_234, DF_235, DF_236, DF_237, DF_238, DF_239, DF_240, DF_241, DF_242, DF_243, DF_244, DF_245, DF_246, DF_247, DF_248, DF_249, DF_250, DF_251, DF_252, DF_253, DF_254, DF_255};





    /*---- mbed_masterからのデータ送信要求の処理 ----*/
    // i=0のとき PING
        void MBED_PING(void) {
            PING_slots.wait();
                val = I2C_master.write(Ping_data,  4);
            PING_slots.release();
            if(!val) led2 = 1;
        }

    // i=1のとき IR
        void MBED_IR(void) {
            Angle_char[3] = (BallCheck) ? 1 : 0;    // ボールの保持を確認
            val = I2C_master.write(Angle_char, 4);  // 0,1:進行方向  2:スピード  3:ボールの保持
                if(!val) led2 = 1;
        }


    // i=2のとき LCD
        void MBED_LCD(void) {
            val = I2C_master.write(lcd, strlen(lcd)+1);
                if(!val) led2 = 1;
        }

    // i=3のとき ロータリースイッチ
        void MBED_ROTARY(void) {
            val = I2C_master.write(Mode, 1);
            if(!val) {
                led2 = 1;
                Flag = 1;
            }
        }

    void (*MBED_ORDER[])(void) = {MBED_PING, MBED_IR, MBED_LCD, MBED_ROTARY};





    // mbedMasterとのI2C通信用プログラム
    void MBED_MASTER(void) {
        int i = I2C_master.receive();
        int j = I2C_avr.receive();

        /* mbed_masterとの通信 */
        switch (i) {
            case I2CSlave::ReadAddressed:   // データ送信命令時の動作
                (*MBED_ORDER[Order])();
                break;
            case I2CSlave::WriteAddressed:  // 命令受信用の動作
                val = I2C_master.read(MBED_data, 1);
                    if(!val)  led1 = 1;
                Order = MBED_data[0];
                break;
            // 基本的に使わない処理
                case I2CSlave::WriteGeneral:    // 念のため一斉通信用の処理
                    break;
                default:
                    break;
        }

        /* avrとの通信 */
        switch (j) {
            case I2CSlave::WriteAddressed:
                val = I2C_avr.read(IR_data, 2);     // 0:near  1:long
                    if(!val)  led3 = 1;
                switch(Mode[0]){
                    case 0: int_char((*FW_AngApi[IR_data[0]])());  break;
                    case 1: int_char((*DF_AngApi[IR_data[0]])());  break;
                    default : break;
                }
                                break;
            // 基本的に使わない処理
                case I2CSlave::ReadAddressed:
                    break;
                case I2CSlave::WriteGeneral:
                    break;
        }
    }




    // int -> char
    void int_char(int x) {
        if (x>255) {    // char一つに収まらないとき
            Angle_char[0] = 255;
            Angle_char[1] = x - 255;
        } else {        // char一つに収まるとき
            Angle_char[0] = x;
            Angle_char[1] = 0;
        }
    }



















