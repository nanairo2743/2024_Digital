// g++ kadai3.cpp -std=c++11 -framework OpenGL -framework GLUT `pkg-config --cflags --libs opencv4` -Wno-deprecated
#include <iostream>           //入出力関連ヘッダ
#include <GLUT/glut.h>        //OpenGL
#include <math.h>             //数学関数
#include <opencv2/opencv.hpp> //OpenCV関連ヘッダ

// 関数名の宣言
void initGL(void);
void display(void);
void reshape(int w, int h);
void timer(int value);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void initCV(void);
void mouseCallback(int event, int x, int y, int flags, void *userdata);

// グローバル変数
double eDist, eDegX, eDegY;                  // 視点極座標
int mX, mY, mState, mButton;                 // マウス座標
int winW, winH;                              // ウィンドウサイズ
double fr = 30.0;                            // フレームレート
cv::VideoCapture capture;                    // ビデオキャプチャ
cv::Size imageSize;                          // 画像サイズ
cv::Mat originalImage, frameImage, optImage; // 画像格納用
double theta = 0.0;
double delta = 1.0; // 回転の速度
int rotFlag = 1;    // 回転フラグ

std::vector<cv::Point> stoc;
std::vector<cv::Point3f> Stoc;
int count = 0;

cv::Mat priorImage(imageSize, CV_8UC1);                // 前フレーム画像(グレースケール)
cv::Mat presentImage(imageSize, CV_8UC1);              // 現フレーム画像(グレースケール)
std::vector<cv::Point2f> priorFeature, presentFeature; // 前フレームおよび現フレーム特徴点
// 反復アルゴリズム停止基準
cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 20, 0.05);
std::vector<unsigned char> status; // 作業用
std::vector<float> errors;         // 作業用

// main関数
int main(int argc, char *argv[])
{
    // OpenGL初期化
    glutInit(&argc, argv);

    // OpenCV初期設定処理
    initCV();

    // OpenGL初期設定処理
    initGL();

    // イベント待ち無限ループ
    glutMainLoop();

    return 0;
}

// OpenCV初期設定処理
void initCV(void)
{
    // ①ビデオキャプチャの初期化
    capture = cv::VideoCapture(0); // カメラ0番をオープン
    if (capture.isOpened() == 0)
    { // オープンに失敗した場合
        printf("Capture not found\n");
        return;
    }

    // ②画像格納用インスタンス準備
    int imageWidth = 720, imageHeight = 405;
    imageSize = cv::Size(imageWidth, imageHeight); // 画像サイズ
    frameImage = cv::Mat(imageSize, CV_8UC3);      // 3チャンネル
    optImage = cv::Mat(imageSize, CV_8UC3);        // 3チャンネル

    // ③画像表示用ウィンドウの生成
    cv::namedWindow("Frame");
    cv::moveWindow("Frame", 0, 0);
    cv::namedWindow("opt");
    cv::moveWindow("opt", 0, 0);

    // マウスコールバック関数のウィンドウへの登録
    cv::setMouseCallback("Frame", mouseCallback);
}

// OpenGL初期設定処理
void initGL(void)
{
    // 初期設定
    glutInitWindowSize(600, 400);                              // ウィンドウサイズ指定
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE); // ディスプレイモード設定

    // OpenGLウィンドウ生成
    glutInitWindowPosition(imageSize.width, 0);
    glutCreateWindow("GL");

    // ウィンドウ消去色設定
    glClearColor(0.9, 0.95, 1.0, 1.0);

    // 機能有効化
    glEnable(GL_DEPTH_TEST); // デプスバッファ
    glEnable(GL_NORMALIZE);  // 法線ベクトル正規化
    glEnable(GL_LIGHTING);   // 陰影付け
    glEnable(GL_LIGHT0);     // 光源０

    // 光原０の設定
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
    eDist = 1500;
    eDegX = 10.0;
    eDegY = 0.0;

    glLineWidth(3.0);
}

// ディスプレイコールバック関数
void display()
{
    // ------------------------------- OpenCV --------------------------------
    //(a)ビデオキャプチャから1フレーム"originalImage"を取り込んで，"frameImage"を生成
    capture >> originalImage;
    // ビデオが終了したら無限ループから脱出
    if (originalImage.data == NULL)
    {
        exit(0);
    }
    //"originalImage"をリサイズして"frameImage"生成
    cv::resize(originalImage, frameImage, imageSize);

    //(b)"frameImage"をグレースケール変換して"presentImage"を生成(現フレーム)
    cv::cvtColor(frameImage, presentImage, cv::COLOR_BGR2GRAY);
    //(c)"priorImage"から特徴点を抽出して"priorFeature[]"に出力
    cv::goodFeaturesToTrack(priorImage, priorFeature, 300, 0.01, 10);
    //(d)オプティカルフロー検出・描画
    int opCnt = priorFeature.size(); // 特徴点数
    if (opCnt > 0)
    { // 特徴点が存在する場合
        // 前フレームの特徴点"priorFeature"から，対応する現フレームの特徴点"presentFeature"を検出
        cv::calcOpticalFlowPyrLK(priorImage, presentImage, priorFeature, presentFeature,
                                 status, errors, cv::Size(10, 10), 5, criteria);

        // スワイプ方向検出用の変数
        int leftSwipeCount = 0;
        int rightSwipeCount = 0;

        // オプティカルフロー描画
        for (int i = 0; i < opCnt; i++)
        {
            cv::Point pt1 = cv::Point(priorFeature[i]);   // 前フレーム特徴点
            cv::Point pt2 = cv::Point(presentFeature[i]); // 現フレーム特徴点

            // 横方向の動きの量を計算
            int dx = pt2.x - pt1.x;
            if (dx > 25)
            {
                rightSwipeCount++;
            }
            else if (dx < -25)
            {
                leftSwipeCount++;
            }

            cv::line(optImage, pt1, pt2, cv::Scalar(0, 0, 255), 1, 8);  // 直線描画
            cv::circle(optImage, pt1, 3, cv::Scalar(0, 255, 0), -1, 8); // 特徴点描画
        }

        // スワイプ方向を標準出力に出力
        if (rightSwipeCount > leftSwipeCount && rightSwipeCount > 50)
        {
            std::cout << "右にスワイプしました" << std::endl;
            for (int i = 0; i < stoc.size(); i++)
            {
                cv::Point3f p = cv::Point3f(stoc[i].x, stoc[i].y, 0);
                Stoc.push_back(p);
            
            }
        }
        else if (leftSwipeCount > rightSwipeCount && leftSwipeCount > 50)
        {
            std::cout << "左にスワイプしました" << std::endl;
            stoc.clear();
            Stoc.clear();
        }
    }

    // 描いた図形を描画（cv::line等で）
    for (int i = 1; i < stoc.size(); ++i)
    {
        cv::line(frameImage, stoc[i - 1], stoc[i], cv::Scalar(255, 255, 255), 2);
    }

    //(b)"frameImage"の表示
    cv::imshow("Frame", frameImage);
    cv::imshow("opt", optImage);

    //(f)現フレームグレースケール画像"presentImage"を前フレームグレースケール画像"priorImage"にコピー
    presentImage.copyTo(priorImage);

    // ------------------------------- OpenGL --------------------------------
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

    //--------------------  地面  --------------------
    // 色設定
    col[0] = 0.5;
    col[1] = 1.0;
    col[2] = 0.5;                                                 // (0.5, 1.0, 0.5) : RGB
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col); // 拡散反射係数
    col[0] = 1.0;
    col[1] = 1.0;
    col[2] = 1.0;
    col[3] = 1.0;
    glMaterialfv(GL_FRONT, GL_SPECULAR, col);
    glMaterialf(GL_FRONT, GL_SHININESS, 64); // ハイライト係数
    glPushMatrix();                          // 行列一時保存
    glScaled(1000, 1, 1000);                 // 拡大縮小
    glutSolidCube(1.0);                      // 立方体の配置
    glPopMatrix();                           // 行列復帰

    // 図形描画(OpenCVで描画した絵（取得したマウス座標（cv::Point型の軌跡）をcv::Point3f型にする)
    col[0] = 0.5;
    col[1] = 1.0;
    col[2] = 0.5;                                                 // (0.5, 1.0, 0.5) : RGB
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col); // 拡散反射係数
    col[0] = 1.0;
    col[1] = 1.0;
    col[2] = 1.0;
    col[3] = 1.0;
    glMaterialfv(GL_FRONT, GL_SPECULAR, col);
    glMaterialf(GL_FRONT, GL_SHININESS, 64); // ハイライト係数
    glPushMatrix();                          // 行列一時保存
    glTranslated(0,100,0);
    glRotated(theta*5,0,1,0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < Stoc.size();i++)
    {
        glVertex3f(Stoc[i].x,Stoc[i].y,Stoc[i].z);
    }
    glEnd();
    glPopMatrix();

    // 描画実行
    glutSwapBuffers();
}

// タイマーコールバック関数
void timer(int value)
{
    if (rotFlag)
    {
        eDegY += delta;
    }

    theta += delta;

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

// マウスコールバック関数 in a window made by OpenCV
void mouseCallback(int event, int x, int y, int flags, void *userdata)
{
    // マウスの座標を出力
    std::cout << "x=" << x << ", y=" << y << " ";

    // イベントの種類を出力
    switch (event)
    {
    case cv::EVENT_MOUSEMOVE:
        std::cout << "マウスが動いた";
        break;
    case cv::EVENT_LBUTTONDOWN:
        std::cout << "左ボタンを押した";
        stoc.push_back(cv::Point(x, y));
        count = count + 1;
        break;
    case cv::EVENT_RBUTTONDOWN:
        std::cout << "右ボタンを押した";
        break;
    case cv::EVENT_LBUTTONUP:
        std::cout << "左ボタンを離した";
        break;
    case cv::EVENT_RBUTTONUP:
        std::cout << "右ボタンを離した";
        break;
    case cv::EVENT_RBUTTONDBLCLK:
        std::cout << "右ボタンをダブルクリック";
        break;
    case cv::EVENT_LBUTTONDBLCLK:
        std::cout << "左ボタンをダブルクリック";
        break;
    }

    // マウスボタンと特殊キーの押下状態を出力
    std::string str;
    if (flags & cv::EVENT_FLAG_ALTKEY)
    {
        str += "Alt "; // ALTキーが押されている
    }
    if (flags & cv::EVENT_FLAG_CTRLKEY)
    {
        str += "Ctrl "; // Ctrlキーが押されている
    }
    if (flags & cv::EVENT_FLAG_SHIFTKEY)
    {
        str += "Shift "; // Shiftキーが押されている
    }
    if (flags & cv::EVENT_FLAG_LBUTTON)
    {
        str += "左ボタン "; // マウスの左ボタンが押されている
    }
    if (flags & cv::EVENT_FLAG_RBUTTON)
    {
        str += "右ボタン"; // マウスの右ボタンが押されている
    }
    if (!str.empty())
    {
        std::cout << "  押下: " << str;
    }
    std::cout << std::endl;
}