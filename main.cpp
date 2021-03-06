#include <QCoreApplication>
#include <cmath>
#include <ctime>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include<stdio.h>
#include <nppcore.h>
#include <nppdefs.h>
#include <nppi.h>
#include <cuda.h>
#include <cuda_runtime.h>

extern "C" void npp_resizeData(cv::Mat srcMat, const int &nRzH, const int &nRzW);

static void help() {
  std::cout
      << "This program demonstrates line finding with the Hough transform."
      << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout
      << "./gpu-example-houghlines <image_name>, Default is ../data/pic1.png\n"
      << std::endl;
}

static void test_cpu_resize_cubic(cv::Mat &srcMat) {
  cv::Mat dstMat;
  clock_t start = clock();
  for (int i = 0; i < 10; ++i) {
    cv::resize(srcMat, dstMat, srcMat.size() * 10, 0, 0, cv::INTER_CUBIC);
  }
  std::cout << "CPU Time : " << 1.0 * (clock() - start) / CLOCKS_PER_SEC << " s"
            << std::endl;
  cv::imwrite(
      "/home/taoxu/Downloads/opencv/samples/data/baboon-cpu-cubic.png",
      dstMat);
}

static void test_gpu_resize_cubic(cv::Mat &srcMat) {
  clock_t start = clock();
  cv::cuda::GpuMat d_srcMat(srcMat);
  cv::cuda::GpuMat d_dstMat;
  for (int i = 0; i < 10; ++i) {
    cv::cuda::resize(d_srcMat, d_dstMat, srcMat.size() * 10, 0, 0,
                     cv::INTER_CUBIC);
  }
  cv::Mat output;
  d_dstMat.download(output);
  std::cout << "GPU Time : " << 1.0 * (clock() - start) / CLOCKS_PER_SEC << " s"
            << std::endl;
  cv::imwrite(
      "/home/taoxu/Downloads/opencv/samples/data/baboon-gpu-cubic.jpg",
      output);
}

int main(int argc, char *argv[]) {

  const std::string filename1(
      "/home/taoxu/Downloads/opencv/samples/data/baboon.jpg");
  cv::Mat srcMat = cv::imread(filename1, cv::IMREAD_COLOR);
  for (int i = 0; i < 1; ++i) {
    test_cpu_resize_cubic(srcMat);
    test_gpu_resize_cubic(srcMat);
  }

  int nRzH = srcMat.rows * 10;
  int nRzW = srcMat.cols * 10;
  clock_t start = clock();
  for (int i = 0; i < 10; ++i) {
      npp_resizeData(srcMat, nRzH, nRzW);
  }
  std::cout << "GPU Time : " << 1.0 * (clock() - start) / CLOCKS_PER_SEC << " s"
            << std::endl;
  return 0;
  const std::string filename = argc >= 2 ? argv[1] : "../data/pic1.png";
  cv::Mat src = cv::imread(filename, cv::IMREAD_GRAYSCALE);
  if (src.empty()) {
    help();
    std::cout << "can not open " << filename << std::endl;
    return -1;
  }

  cv::Mat mask;
  cv::Canny(src, mask, 100, 200, 3);

  cv::Mat dst_cpu;
  cv::cvtColor(mask, dst_cpu, cv::COLOR_GRAY2BGR);
  cv::Mat dst_gpu = dst_cpu.clone();

  std::vector<cv::Vec4i> lines_cpu;
  {
    const int64 start = cv::getTickCount();

    cv::HoughLinesP(mask, lines_cpu, 1, CV_PI / 180, 50, 60, 5);

    const double timeSec =
        (cv::getTickCount() - start) / cv::getTickFrequency();
    std::cout << "CPU Time : " << timeSec * 1000 << " ms" << std::endl;
    std::cout << "CPU Found : " << lines_cpu.size() << std::endl;
  }

  for (size_t i = 0; i < lines_cpu.size(); ++i) {
    cv::Vec4i l = lines_cpu[i];
    cv::line(dst_cpu, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]),
             cv::Scalar(0, 0, 255), 3, cv::LINE_AA);
  }

  cv::cuda::GpuMat d_src(mask);
  cv::cuda::GpuMat d_lines;
  {
    const int64 start = cv::getTickCount();

    cv::Ptr<cv::cuda::HoughSegmentDetector> hough =
        cv::cuda::createHoughSegmentDetector(1.0f, (float)(CV_PI / 180.0f), 50,
                                             5);

    hough->detect(d_src, d_lines);

    const double timeSec =
        (cv::getTickCount() - start) / cv::getTickFrequency();
    std::cout << "GPU Time : " << timeSec * 1000 << " ms" << std::endl;
    std::cout << "GPU Found : " << d_lines.cols << std::endl;
  }
  std::vector<cv::Vec4i> lines_gpu;
  if (!d_lines.empty()) {
    lines_gpu.resize(d_lines.cols);
    cv::Mat h_lines(1, d_lines.cols, CV_32SC4, &lines_gpu[0]);
    d_lines.download(h_lines);
  }

  for (size_t i = 0; i < lines_gpu.size(); ++i) {
    cv::Vec4i l = lines_gpu[i];
    cv::line(dst_gpu, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]),
             cv::Scalar(0, 0, 255), 3, cv::LINE_AA);
  }

  cv::imshow("source", src);
  cv::imshow("detected lines [CPU]", dst_cpu);
  cv::imshow("detected lines [GPU]", dst_gpu);
  cv::waitKey(0);
  return 0;
}
