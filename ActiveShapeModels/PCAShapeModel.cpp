#include "PCAShapeModel.h"

//debug
#include <iostream>
using std::cout;
using std::endl;
#include "ResultProcessor.h"
#include <opencv2\opencv.hpp>
//end debug

PCAShapeModel::PCAShapeModel(void)
{
}


PCAShapeModel::~PCAShapeModel(void)
{
}

void PCAShapeModel::mergeXY(const cv::Mat &X, const cv::Mat &Y, cv::Mat &XY){
	const int rows = X.rows, cols = X.cols;
	XY = cv::Mat(rows * 2, cols, X.type(), cv::Scalar::all(0));
	
	//cannot copy like following code
	//XY(cv::Range(0, rows - 1), cv::Range(0, cols - 1)) = X;
	//XY(cv::Range(rows, rows * 2 - 1), cv::Range(0, cols -1)) = Y;
	//should like following code
	cv::Mat XYU(XY(cv::Range(0, rows), cv::Range(0, cols)));
	cv::Mat XYD(XY.rowRange(rows, rows * 2));
	X(cv::Range(0, rows), cv::Range(0, cols)).copyTo(XYU);
	Y.rowRange(0, rows).copyTo(XYD);

	//Range(start, end)  end should be the real value + 1

	//debug
	//cout << "X.col(0)" << endl << X.col(0) << endl;
	//cout << "Y.col(0)" << endl << Y.col(0) << endl;
	//cout << "XY.col(0)" << endl << XY.col(0) << endl;
	//end debug
}

void PCAShapeModel::splitXY(const cv::Mat &XY, cv::Mat &X, cv::Mat &Y){
	const int rows = XY.rows;
	int rows2 = rows / 2;
	X = XY.rowRange(cv::Range(0, rows2));
	Y = XY.rowRange(cv::Range(rows2, rows));

	//debug
	//cout << X << endl;
	//end debug
}

void PCAShapeModel::generateBases(const cv::Mat &alignedShapesX, const cv::Mat &alignedShapesY, 
				   const cv::Mat &meanShapeX, const cv::Mat &meanShapeY){
	
	const int numberOfShapes = alignedShapesX.cols;
	const int numberOfPoints = alignedShapesX.rows;

	cv::Mat shapesXY;
	mergeXY(alignedShapesX, alignedShapesY, shapesXY);

	cv::Mat meanShapeXY;
	mergeXY(meanShapeX, meanShapeY, meanShapeXY);

	PCAShapeModel::meanShapeX = meanShapeX;
	PCAShapeModel::meanShapeY = meanShapeY;

	pca = cv::PCA(shapesXY,
				meanShapeXY,
				CV_PCA_DATA_AS_COL,
				c_retainedVariance);
}

void PCAShapeModel::findBestDeforming(const cv::Mat &X0, const cv::Mat &Y0,
	const cv::Mat &sX, const cv::Mat &sY, const MappingParameters &_para, 
	const cv::Mat &WInOneColumn, const cv::Mat &W, cv::Mat &resX, cv::Mat &resY){
	
	MappingParameters para = _para;
	para.inverse();
	
	cv::Mat X, Y;
	para.getAlignedXY(X0, Y0, X, Y, -1.0);
	X += (1.0 / para.scale) * sX;
	Y += (1.0 / para.scale) * sY;

	MappingParameters para2MeanShape;
	AlignShape alignShape;

	para2MeanShape = alignShape.findBestMapping(meanShapeX, meanShapeY, X, Y, WInOneColumn, W);
	para2MeanShape.getAlignedXY(X, Y, X, Y);

	cv::Mat XY;
	mergeXY(X, Y, XY);

	cv::Mat b;
	pca.project(XY, b);

	const int numberOfComponents = b.rows;

	for(int i = 0; i < numberOfComponents; i++){
		double threshold = 3.0 * sqrt(abs(pca.eigenvalues.at<double>(i)));
		if(b.at<double>(i) < -threshold){
			b.at<double>(i) = -threshold;
		}

		if(b.at<double>(i) > threshold){
			b.at<double>(i) = threshold;
		}
	}

	//debug
	//cout << "b:" << endl;
	//cout << b << endl;
	//cout << "eigenvalues: " << endl;
	//cout << pca.eigenvalues << endl;
	//end debug

	pca.backProject(b, XY);

	splitXY(XY, resX, resY);

	//debug
	//cout << "resX _ old : " << endl;
	//cout << resX << endl;
	//cout << "resY _ old : " << endl;
	//cout << resY << endl;
	ResultProcessor resultProcessor;
	cv::Mat tmpMat = cv::Mat(640, 480, CV_32F, cv::Scalar::all(0));
	resultProcessor.showResultImage(resX, resY, tmpMat, "pca result");
	//end debug

	para2MeanShape.inverse();
	para2MeanShape.getAlignedXY(resX, resY, resX, resY);

	//debug
	cout << "resX _ new: " << endl;
	cout << resX << endl;
	cout << "resY _ new : " << endl;
	cout << resY << endl;
	//end debug
}