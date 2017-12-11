#include "detecter.h"
#include <QDebug>
#include <vector>
#include <QTime>
#include <opencv2/highgui.hpp>

Detecter::Detecter()
{
    ori_img = new Mat;
    DFT_img = new Mat;
    p_DFT_img = new Mat;
    p_img = new Mat;
    bw_img = new Mat;
}

Detecter::~Detecter()
{
    delete ori_img;
    delete DFT_img;
    delete p_DFT_img;
    delete p_img;
    delete bw_img;
//    qDebug() << "delete done!";
}

void Detecter::set_img(const Mat &data)
{
    *ori_img = data.clone();
    img_width = data.cols;
    img_height = data.rows;
    main_task();
}

void Detecter::set_img(const Mat* data)
{
    *ori_img = data->clone();
    img_width = data->cols;
    img_height = data->rows;
    main_task();
}

void Detecter::main_task()
{
    complex_Mat = create_complex_Mat(*ori_img);
    DFT_function(complex_Mat,Am,Cosine,Sine);
    *DFT_img = get_energyMap(Am);
    Mat mask = get_sailencyMap(*DFT_img);
    mask = 1 - mask;
    multiply(*DFT_img,mask,*p_DFT_img);
    ishift(mask);
//    qDebug() << Am.type();
    mask.convertTo(mask,CV_32F);
    multiply(mask,Am,Am);
    IDFT_function(*p_img,Am,Cosine,Sine);
    bw_t = get_bw_value(*p_img,10);
    threshold(*p_img,*bw_img,bw_t,255,CV_THRESH_BINARY);
    bwareaopen(*bw_img,4);
    *bw_img = (*bw_img) < 100;

}

Mat Detecter::create_complex_Mat(Mat &data)
{
    Mat output;
    Mat r = data.clone();
//    for(int i=0;i<2;i++)
//        qDebug() << r.ptr<uchar>(0)[i];
//    for(int i=0;i<2;i++)
//        qDebug() << r.ptr<uchar>(1)[i];
    Mat i = Mat::zeros(r.size(),CV_32F);
    if(r.type() != 5)
        r.convertTo(r,CV_32F);
//    qDebug() << r.channels() << r.isContinuous();
    Mat temp[] = {r,i};
    merge(temp,2,output);
//    qDebug() << output.isContinuous();
//    qDebug() << output.channels();
//    for(int i=0;i<6;i++)
//        qDebug() << output.ptr<float>(0)[i];
    return output;
}

Mat Detecter::create_complex_Mat(Mat &R, Mat &I)
{
    Mat r = R.clone();
    Mat i = I.clone();
    if(r.type() != 5)
        r.convertTo(r,CV_32F);
    if(i.type() != 5)
        i.convertTo(i,CV_32F);
    Mat output;
    Mat temp[] = {r, i};
    merge(temp,2,output);
//    for (int i=0;i<4;i++)
//        qDebug() << output.ptr<float>(0)[i];
    return output;
}

void Detecter::DFT_function(Mat &data, Mat &Am, Mat &Cosine, Mat &Sine)
{
    dft(data,data);
//    qDebug() << data.type() << data.ptr<float>(0)[0] << data.ptr<float>(0)[1];
    Mat temp[2];
    split(data,temp);
    Mat R = temp[0].clone();
    Mat I = temp[1].clone();
    magnitude(R,I,Am);
    divide(R, Am, Cosine);
    divide(I, Am, Sine);
    Am += 1;
    log(Am,Am);
//    for(int i=0;i<3;i++)
//    {
//        qDebug() << R.ptr<float>(0)[i];
//        qDebug() << I.ptr<float>(0)[i];
//    }
}

void Detecter::IDFT_function(Mat &output, Mat Am, Mat Cosine, Mat Sine)
{
    exp(Am,Am);
    Mat R, I;
    multiply(Am,Cosine,R);
    multiply(Am,Sine,I);
    Mat complex = create_complex_Mat(R, I);
    idft(complex,complex);
    Mat temp[2];
    split(complex,temp);
    R = temp[0];
    I = temp[1];
    magnitude(R,I,output);
    multiply(output,output,output);
    normalize(output,output,0,255,NORM_MINMAX);
    output.convertTo(output,CV_8UC1);
}

Mat Detecter::get_energyMap(Mat Am)
{
    Mat output = Am.clone();
//    qDebug() << output.ptr<float>(0)[0];
    shift(output);
//    qDebug() << output.ptr<float>(0)[0] << output.ptr<float>(0)[1] << output.ptr<float>(0)[2];
    normalize(output,output,0,255,NORM_MINMAX);
//    qDebug() << output.ptr<float>(0)[0] << output.ptr<float>(0)[1] << output.ptr<float>(0)[2];
    output.convertTo(output,CV_8UC1);
//    qDebug() << output.ptr<uchar>(0)[0];
    return output;
}

void Detecter::shift(Mat &data)
{
    int cx = data.cols/2;
    int cy = data.rows/2;
    Mat temp;
    Mat q0(data, Rect(0, 0, cx, cy));
    Mat q1(data, Rect(cx, 0, cx, cy));
    Mat q2(data, Rect(0, cy, cx, cy));
    Mat q3(data, Rect(cx, cy, cx, cy));

    q0.copyTo(temp);
    q3.copyTo(q0);
    temp.copyTo(q3);

    q1.copyTo(temp);
    q2.copyTo(q1);
    temp.copyTo(q2);
}

void Detecter::ishift(Mat &data)
{
    int cx = data.cols/2;
    int cy = data.rows/2;
    Mat temp;
    Mat q0(data, Rect(0, 0, cx, cy));
    Mat q1(data, Rect(cx, 0, cx, cy));
    Mat q2(data, Rect(0, cy, cx, cy));
    Mat q3(data, Rect(cx, cy, cx, cy));

    q0.copyTo(temp);
    q3.copyTo(q0);
    temp.copyTo(q3);

    q1.copyTo(temp);
    q2.copyTo(q1);
    temp.copyTo(q2);
}

Mat Detecter::get_sailencyMap(Mat &data)
{
//    qDebug() << data.ptr<uchar>(0)[0] << data.ptr<uchar>(0)[1] << data.ptr<uchar>(1)[0] << data.ptr<uchar>(1)[1];
    Mat complex = create_complex_Mat(data);
    Mat A, C, S;
    DFT_function(complex,A,C,S);
    Mat A_avg;
    blur(A,A_avg,Size(11,11),Point(-1,-1),BORDER_REPLICATE);
//    qDebug() << A_avg.ptr<float>(0)[0];
    A = A - A_avg;
//    for(int i=0;i<4;i++)
//        qDebug() << A.ptr<float>(0)[i];
    IDFT_function(A,A,C,S);
//    qDebug() << A.channels() << A.type();
//    for(int i=0;i<4;i++)
//        qDebug() << A.ptr<float>(0)[i];
    threshold(A,A,0,1,CV_THRESH_OTSU);
    bwareaopen(A,4);
    Mat kern = getStructuringElement(MORPH_CROSS, Size(3, 3));
    dilate(A, A, kern, Point(-1, -1), 2);
    circle(A,cv::Point((img_width/2),(img_height/2)),5,0,-1);
    circle(A,cv::Point((img_width/2),(img_height/2)),0,1,-1);
    return A;
}

void Detecter::bwareaopen(Mat &data, int n)
{
    Mat labels,stats,centroids;
    connectedComponentsWithStats(data,labels,stats,centroids,8,CV_16U);
    int regions_count = stats.rows - 1;
    int regions_size, regions_x1, regions_y1, regions_x2, regions_y2;

    for(int i=1;i<=regions_count;i++)
    {
        regions_size = stats.ptr<int>(i)[4];
        if(regions_size < n)
        {
            regions_x1 = stats.ptr<int>(i)[0];
            regions_y1 = stats.ptr<int>(i)[1];
            regions_x2 = regions_x1 + stats.ptr<int>(i)[2];
            regions_y2 = regions_y1 + stats.ptr<int>(i)[3];

            for(int j=regions_y1;j<regions_y2;j++)
            {
                for(int k=regions_x1;k<regions_x2;k++)
                {
                    if(labels.ptr<ushort>(j)[k] == i)
                        data.ptr<uchar>(j)[k] = 0;
                }
            }
        }
    }
}

double Detecter::get_bw_value(const Mat &input,double t)
{
    int channels[] = { 0 };
    int histSize[] = {256};
    float my_ranges[] = { 0,256 };
    const float* ranges[] = { my_ranges };
    cv::MatND hist;
    calcHist(&input, 1, channels, cv::Mat(), hist, 1, histSize, ranges);
//    qDebug() << hist.channels() << hist.rows << hist.cols;
//    qDebug() << hist.ptr<float>(0)[0];
    Mat mean_value, std_value;
    int i;
    for(i=0;i<256;i++)
    {
        meanStdDev(hist.rowRange(i,255),mean_value,std_value);
//        qDebug() << var_value.type();
//        qDebug() << double(var_value.ptr<float>(0)[0]);
        if(std_value.ptr<double>(0)[0] < t)
            break;
    }
//    qDebug() << i;
    return double(i);
}