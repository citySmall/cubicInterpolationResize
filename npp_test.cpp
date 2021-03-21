#include <nppcore.h>
#include <nppdefs.h>
#include <nppi.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <cstdio>

extern "C" void npp_resizeData(cv::Mat srcMat, const int &nRzH, const int &nRzW);

void npp_resizeData(cv::Mat srcMat, const int &nRzH, const int &nRzW) {
    int nH = srcMat.rows;
    int nW = srcMat.cols;
    int nC = srcMat.channels();
    int nStep = srcMat.step;
    // printf("nH = %d, nW = %d, nC = %d, nStep = %d\n", nH, nW, nC, nStep);
    // 1. 将图像数据拷贝到设备端
    Npp8u *pu8srcData_dev = NULL;
    cudaMalloc((void **)&pu8srcData_dev, nH * nW * nC * sizeof(Npp8u));
    cudaMemcpy(pu8srcData_dev, srcMat.data, nH * nW * nC * sizeof(Npp8u),
               cudaMemcpyHostToDevice);

    // 2. 在设备端开辟空间
    Npp8u *pu8dstData_dev = NULL;
    NppiSize npp_srcSize{nW, nH};
    NppiSize npp_dstSize{nRzW, nRzH};
    cudaMalloc((void **)&pu8dstData_dev, nRzH * nRzW * nC * sizeof(Npp8u));
    cudaMemset(pu8dstData_dev, 0, nRzH * nRzW * nC * sizeof(Npp8u));
    // 3.调用nppiresize函数
    nppiResize_8u_C3R((Npp8u *)pu8srcData_dev, nStep, npp_srcSize,
                      NppiRect{0, 0, nW, nH}, (Npp8u *)pu8dstData_dev, nRzW * 3,
                      npp_dstSize, NppiRect{0, 0, nRzW, nRzH}, NPPI_INTER_CUBIC);

    // 将resize后的图像内存（设备端）拷贝到host端
    cv::Mat newimage(nRzH, nRzW, CV_8UC3);
    cudaMemcpy(newimage.data, pu8dstData_dev, nRzH * nRzW * 3,
               cudaMemcpyDeviceToHost);
    if (pu8dstData_dev != NULL) {
        cudaFree(pu8dstData_dev);
        pu8dstData_dev = NULL;
    }
    if (pu8srcData_dev != NULL) {
        cudaFree(pu8srcData_dev);
        pu8srcData_dev = NULL;
    }
    // 保存图像，验证结果
    // cv::imwrite("/home/taoxu/Downloads/opencv/samples/data/baboon-npp.png", newimage);
}
