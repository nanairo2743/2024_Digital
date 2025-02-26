#include <iostream>
#include <opencv2/opencv.hpp>

// 顔色の変更
void change_face_color(cv::Mat& faceImage, cv::Mat& hsvImage, cv::Rect rect)
{
    // 色解析しやすいようにHSV色空間に変換
    cv::cvtColor(faceImage, hsvImage, cv::COLOR_BGR2HSV);

    for(int j=rect.y; j<rect.y+rect.height; j++) {
        if(j<0 || j>= hsvImage.rows) continue;
        for(int i=rect.x; i<rect.x+rect.width; i++) {
            if(i<0 || i>= hsvImage.cols) continue;
            cv::Vec3b s = hsvImage.at<cv::Vec3b>(j, i);
            hsvImage.at<cv::Vec3b>(j, 2*rect.x+rect.width-i) = s;
            // 肌色領域のみ変換
            if(s[0]> 0 && s[0]< 45 &&
                s[1]>50 && s[1]<255 &&
                s[2]>50 && s[2]<255)
            {
                s[0] = 120;
                hsvImage.at<cv::Vec3b>(j, i) = s;
            }
        }
    }
    cv::cvtColor(hsvImage, faceImage, cv::COLOR_HSV2BGR);
}

//main関数
int main(int argc, char* argv[])
{
    //OpenCV初期設定処理
    //カメラキャプチャの初期化
    cv::VideoCapture capture(0);
    if (capture.isOpened()==0) {
        //カメラが見つからないときはメッセージを表示して終了
        printf("Camera not found\n");
        exit(1);
    }

    cv::Mat originalImage, frameImage, hsvImage, tempImage;
    cv::Size imageSize(720, 405);  // 画像サイズ
    cv::CascadeClassifier faceClassifier;  // 顔認識用分類器
    cv::CascadeClassifier eyeClassifier;  // 目認識用分類器

    //3チャンネル画像"hsvImage"と"tempImage"の確保（ビデオと同サイズ）
    hsvImage = cv::Mat(imageSize, CV_8UC3);
    tempImage = cv::Mat(imageSize, CV_8UC3);

    //OpenCVウィンドウ生成
    cv::namedWindow("Frame");
    cv::moveWindow("Frame", 0, 0);
    cv::namedWindow("Face");
    cv::moveWindow("Face", imageSize.width, 0);
    cv::namedWindow("usamimiImage");

    // ①正面顔検出器の読み込み
    faceClassifier.load("haarcascades/haarcascade_frontalface_default.xml");

    // 眼検出器の読み込み
    eyeClassifier.load("haarcascades/haarcascade_mcs_eyepair_big.xml");

    // うさみみ画像の読み込み
    cv::Mat usamimiImage = cv::imread("うさミミ.jpg");

    while(1){
        //ビデオキャプチャから1フレーム画像取得
        capture >> originalImage;

        //ビデオが終了したら巻き戻し
        if(originalImage.data==NULL) {
            capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            continue;
        }

        cv::resize(originalImage, frameImage, imageSize);

        //フレーム画像表示
        cv::imshow("Frame", frameImage);

        // ②検出情報を受け取るための配列を用意する
        std::vector<cv::Rect> faces, eyes;

        // ③画像中から検出対象の情報を取得する
        faceClassifier.detectMultiScale(frameImage, faces, 1.1, 3, 0, cv::Size(20,20));
        eyeClassifier.detectMultiScale(frameImage, eyes, 1.1, 3, 0, cv::Size(10,10));

        // ④顔領域の検出
        for (int i = 0; i < faces.size(); i++) {
            // 検出情報から顔の位置情報を取得
            cv::Rect face = faces[i];
            // 大きさによるチェック。
            if(face.width*face.height < 100*100){
                continue; // 小さい矩形は採用しない
            }

            // うさみみ画像のリサイズ(画像の横幅を顔の横幅に合わせる、縦幅はアスペクト比を保持してリサイズ)
            cv::resize(usamimiImage, tempImage, cv::Size(face.width, face.width * usamimiImage.rows / usamimiImage.cols));

            cv::imshow("usamimiImage", tempImage);

            // うさみみの背景は白色なので、白(255,255,255)以外をマスクし、copyToで合成
            cv::Mat mask;
            cv::cvtColor(tempImage, mask, cv::COLOR_BGR2GRAY);
            cv::threshold(mask, mask, 245, 255, cv::THRESH_BINARY_INV);
            cv::Rect targetROI(face.x, face.y - face.width * usamimiImage.rows / usamimiImage.cols, face.width, face.width * usamimiImage.rows / usamimiImage.cols);

            // 範囲チェック
            if (targetROI.x >= 0 && targetROI.y >= 0 && 
                targetROI.x + targetROI.width <= frameImage.cols && 
                targetROI.y + targetROI.height <= frameImage.rows) 
            {
                tempImage.copyTo(frameImage(targetROI), mask);
            }
        }

        //認識結果画像表示
        cv::imshow("Face", frameImage);

        char key = cv::waitKey(10);
        if(key == 'q'){
            break;
        }
    }

    return 0;
}
