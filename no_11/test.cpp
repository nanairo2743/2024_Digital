// g++ dip11_glcv.cpp -std=c++11 -framework OpenGL -framework GLUT `pkg-config --cflags --libs opencv4` -Wno-deprecated
#include <iostream>           //入出力関連ヘッダ
#include <GLUT/glut.h>        //OpenGL
#include <opencv2/opencv.hpp> //OpenCV関連ヘッダ
#include <math.h>             //数学関数

// 関数名の宣言
void initGL();
void display();
void reshape(int w, int h);
void timer(int value);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);

// グローバル変数
double eDist, eDegX, eDegY;  // 視点極座標
int mX, mY, mState, mButton; // マウス座標
int winW, winH;              // ウィンドウサイズ
double fr = 30.0;            // フレームレート」

double angle1 = 0.0; // 直方体1の回転角度
double angle2 = 0.0; // 直方体2の回転角度

cv::VideoCapture capture;

int Width = 800, Height = 600;
cv::Size imageSize(Width, Height);      // 画像サイズ
cv::Mat originalImage;                  // ビデオキャプチャ用
cv::Mat frameImage(imageSize, CV_8UC3); // 3チャンネル

// main関数
int main(int argc, char *argv[])
{
    // opencv
    //  ①ビデオキャプチャの初期化
    capture = cv::VideoCapture(0); // カメラ0番をオープン
    if (capture.isOpened() == 0)
    { // オープンに失敗した場合
        printf("Capture not found\n");
        return -1;
    }

    // ②画像格納用インスタンス準備
    imageSize = cv::Size(Width, Height);      // 画像サイズ                      // ビデオキャプチャ用
    frameImage = cv::Mat(imageSize, CV_8UC3); // 3チャンネル

    // ③画像表示用ウィンドウの生成
    cv::namedWindow("Frame");
    cv::moveWindow("Frame", 0, 0);

    // openGL
    //  OpenGL初期化
    glutInit(&argc, argv);

    // OpenGL初期設定処理
    initGL();

    // イベント待ち無限ループ
    glutMainLoop();

    return 0;
}

// OpenGL初期設定処理
void initGL()
{
    // 初期設定
    glutInitWindowSize(800, 600);                              // ウィンドウサイズ指定
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE); // ディスプレイモード設定

    // OpenGLウィンドウ生成
    glutCreateWindow("GL");
    glutInitWindowPosition(0, 0);

    // ウィンドウ消去色設定
    glClearColor(0.0, 0.0, 0.2, 1.0);

    // 機能有効化
    glEnable(GL_DEPTH_TEST); // デプスバッファ
    glEnable(GL_NORMALIZE);  // 法線ベクトル正規化
    glEnable(GL_LIGHTING);   // 陰影付け

    // 光原設定
    GLfloat col[4];      // パラメータ(RGBA)
    glEnable(GL_LIGHT0); // 光源0
    col[0] = 0.9;
    col[1] = 0.9;
    col[2] = 0.9;
    col[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, col);  // 光源0の拡散反射の強度
    glLightfv(GL_LIGHT0, GL_SPECULAR, col); // 光源0の鏡面反射の強度
    col[0] = 0.05;
    col[1] = 0.05;
    col[2] = 0.05;
    col[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_AMBIENT, col); // 光源0の環境光の強度

    // コールバック関数
    glutDisplayFunc(display);           // ディスプレイコールバック関数の指定
    glutReshapeFunc(reshape);           // リシェイプコールバック関数の指定
    glutMouseFunc(mouse);               // マウスクリックコールバック関数の指定
    glutMotionFunc(motion);             // マウスドラッグコールバック関数の指定
    glutKeyboardFunc(keyboard);         // キーボードコールバック関数の指定
    glutTimerFunc(1000 / fr, timer, 0); // タイマーコールバック関数の指定

    // 視点極座標初期値
    eDist = 1200;
    eDegX = 0.0;
    eDegY = 0.0;
}

// ディスプレイコールバック関数
void display()
{
    // opencv
    //(a)ビデオキャプチャから1フレーム"originalImage"を取り込んで，"frameImage"を生成
    capture >> originalImage;
    // ビデオが終了したら無限ループから脱出
    if (originalImage.data == NULL)
    {
        exit(0);
    }
    //"originalImage"をリサイズして"frameImage"生成
    cv::resize(originalImage, frameImage, imageSize);

    //(b)"frameImage"の表示
    cv::imshow("Frame", frameImage);

    // OpenGL用の形式に変換
    cv::cvtColor(frameImage, frameImage, cv::COLOR_BGR2RGB);

    // モノクロに変換
    cv::cvtColor(frameImage, frameImage, cv::COLOR_RGB2GRAY);

    // 32*24のドットに変換
    cv::resize(frameImage, frameImage, cv::Size(32, 24), 0, 0, cv::INTER_NEAREST);

    //(c)キー入力待ち
    int key = cv::waitKey(10);
    //[Q]が押されたら無限ループ脱出
    if (key == 'q')
        exit(0);

    // openGL
    GLfloat col[4]; // 色設定用

    // ウィンドウ内消去
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 行列初期化
    glLoadIdentity();

    // 視点座標の計算
    double ex = eDist * cos(eDegX * M_PI / 180.0) * sin(eDegY * M_PI / 180.0);
    double ey = eDist * sin(eDegX * M_PI / 180.0);
    double ez = eDist * cos(eDegX * M_PI / 180.0) * cos(eDegY * M_PI / 180.0);

    // 視点視線の設定
    gluLookAt(ex, ey, ez, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // 変換行列に視野変換行列を乗算

    // 光源0の位置指定
    GLfloat pos0[] = {200.0, 700.0, 200.0, 0.0}; //(x, y, z, 0(平行光源)/1(点光源))
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);

    //--------------------直方体--------------------
    // 色設定
    for (int y = 0; y < 24; y++)
    {
        for (int x = 0; x < 32; x++)
        {
            // 画素値を取得
            int val = frameImage.at<uchar>(y, x);

            double angle = ((double)90 / 255.0) * val - 45.0;

            // 色設定
            col[0] = 0.9;
            col[1] = 0.5;
            col[2] = 0.0;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, col);
            // 長方形の描画
            glPushMatrix(); // 行列保存
            glTranslatef(Width / 2 - 25 * x, Height / 2 - 25 * y, 0);
            glRotated(angle, 0.0, 0.0, 0.0); // x軸周りに回転
            glScalef(1.0, 1.0, 0.5);         // 長方形の厚みを変更
            glutSolidCube(20);               // 長方形の描画
            glPopMatrix();                   // 行列復帰
        }
    }

    // 描画実行
    glutSwapBuffers();
}

// タイマーコールバック関数
void timer(int value)
{

    angle1 += 2.0; // 直方体1の回転角度を更新
    angle2 += 3.0; // 直方体2の回転角度を更新

    if (angle1 >= 360.0)
        angle1 -= 360.0;
    if (angle2 >= 360.0)
        angle2 -= 360.0;

    glutPostRedisplay();                // ディスプレイイベント強制発生
    glutTimerFunc(1000 / fr, timer, 0); // タイマー再設定
}

// リシェイプコールバック関数
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);                                    // ウィンドウ全体が描画対象
    glMatrixMode(GL_PROJECTION);                               // 投影変換行列を計算対象に設定
    glLoadIdentity();                                          // 行列初期化
    gluPerspective(30.0, (double)w / (double)h, 1.0, 10000.0); // 変換行列に透視投影を乗算
    glMatrixMode(GL_MODELVIEW);                                // モデルビュー変換行列を計算対象に設定
}

// マウスクリックコールバック関数
void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
        // マウス情報格納
        mX = x;
        mY = y;
        mState = state;
        mButton = button;
    }
}

// マウスドラッグコールバック関数
void motion(int x, int y)
{
    if (mButton == GLUT_RIGHT_BUTTON)
    {
        // マウスの移動量を角度変化量に変換
        eDegY = eDegY + (mX - x) * 0.5; // マウス横方向→水平角
        eDegX = eDegX + (y - mY) * 0.5; // マウス縦方向→垂直角
    }

    // マウス座標格納
    mX = x;
    mY = y;
}

// キーボードコールバック関数(key:キーの種類，x,y:座標)
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'q':
    case 'Q':
    case 27:
        exit(0);
    }
}
