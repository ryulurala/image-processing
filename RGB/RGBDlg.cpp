
// RGBDlg.cpp : 구현 파일
//

#include "pch.h" // #include "stdafx.h"
#include "RGB.h"
#include "RGBDlg.h"
#include "afxdialogex.h"
#include <malloc.h>
#include <math.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console") // console

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
void ChangeColor(Mat img, Mat &copy, int i);
Mat Img2red(Mat img);
Mat Img2green(Mat img);
Mat Img2blue(Mat img);
Mat Img2gray(Mat img);
Mat Img2otsu(Mat img);
Mat Img2dilation(Mat img, Mat kernel);
Mat Img2erosion(Mat img, Mat kernel);
Mat Img2opening(Mat img, Mat kernel);
Mat Img2closing(Mat img, Mat kernel);

int kernel_rows = 5, kernel_cols = 5;
Mat kernel = Mat::zeros(kernel_rows, kernel_cols, CV_8UC1);

std::vector<double> thresArray;
std::vector<double> sortThres;

std::pair<int, int> _xy;								   // (x, y) temporary
std::vector<std::pair<int, int>> _xyList;				   // Contour의 (x, y)
std::vector<std::vector<std::pair<int, int>>> contourList; // Image당 Contour

std::vector<int> _path;
std::vector<std::vector<int>> _imgPath; // Path

Mat Img2contour(Mat img);
Mat LabelingwithBT(Mat binaryImg, Mat &contourImg);
void BTracing8(int y, int x, int label, int tag, Mat binImg, Mat &labImg, Mat &boundImg, int **LUT_BLabeling);
void CalcOrd(int i, int *y, int *x);
void Read_neighbor8(int y, int x, int *neighbor8, Mat img);
int **InitLUT(int size);
double *ExtractLCS();
double FindDistance(int contourIndex, int pairIndex, int window);
double DTWarping(double *LCS1, double *LCS2, double *deviPatt);
int ForDTW_MinArg(int i, int j, double **&dMat);
void StatisticalDTW(double **trainImgLCSs, double *&newPatt, double *&deviPatt);
void ForSDTW_Average(double **trainImgLCSs, double *&avgPatt);
void ForSDTW_Deviation(double **trainImgLCSs, double *avgPatt, double *&deviPatt);
double FindThres(double *curPatt, double *prePatt);

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum
	{
		IDD = IDD_ABOUTBOX
	};
#endif

protected:
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CRGBDlg 대화 상자

CRGBDlg::CRGBDlg(CWnd *pParent /*=NULL*/)
	: CDialogEx(IDD_RGB_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRGBDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_Img, m_pic);
}

BEGIN_MESSAGE_MAP(CRGBDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDC_TRAININGLOAD, &CRGBDlg::OnBnClickedTrainingload)
ON_BN_CLICKED(IDC_TESTLOAD, &CRGBDlg::OnBnClickedTestload)
ON_BN_CLICKED(IDC_SAVERESULT, &CRGBDlg::OnBnClickedSaveresult)
ON_BN_CLICKED(IDC_TRAINING, &CRGBDlg::OnBnClickedTraining)
END_MESSAGE_MAP()

// CRGBDlg 메시지 처리기

BOOL CRGBDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu *pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);	 // 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE); // 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	m_trSize = 0;
	m_teSize = 0;
	m_classCount = 0;

	return TRUE; // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CRGBDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CRGBDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CRGBDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRGBDlg::OnBnClickedTrainingload()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	static TCHAR BASED_CODE szFilter[] = _T("이미지 파일(*.BMP, *.GIF, *.JPG) | *.BMP;*.GIF;*.JPG;*.bmp;*.jpg;*.gif |모든파일(*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("*.jpg"), _T("image"), OFN_ALLOWMULTISELECT, szFilter);

	if (IDOK == dlg.DoModal())
	{
		m_classCount = 0;

		for (POSITION pos = dlg.GetStartPosition(); pos != NULL;)
		{
			pathName = dlg.GetNextPathName(pos);
			CT2CA pszConvertedAnsiString_up(pathName);
			std::string up_pathName_str(pszConvertedAnsiString_up);
			m_className.push_back(up_pathName_str);
			m_trainImg[m_trSize++] = cv::imread(up_pathName_str);
			m_classCount++;
		}

		GetDlgItem(IDC_TRAINING)->EnableWindow(TRUE);
	}
}

void CRGBDlg::OnBnClickedTraining()
{
	// Kernel init
	for (int j = 0; j < kernel_rows; j++)
	{
		for (int i = 0; i < kernel_cols; i++)
		{
			kernel.at<uchar>(j, i) = 255;
		}
	}

	String msg; // winname

	double **trainImgLCSs = (double **)calloc(m_classCount, sizeof(double *));
	double *newPatt;
	double *deviPatt;
	int classIndex;

	for (int i = 0; i < m_trSize; i++)
	{
		// Image Processing
		classIndex = i % m_classCount;

		// GrayScale
		m_trainImg[i] = Img2gray(m_trainImg[i]);

		// BinaryImage
		m_trainImg[i] = Img2otsu(m_trainImg[i]);

		// Opening or Closing
		m_trainImg[i] = Img2opening(m_trainImg[i], kernel);
		//m_trainImg[i] = Img2closing(m_trainImg[i], kernel);

		// Contour tracing
		m_trainImg[i] = Img2contour(m_trainImg[i]);

		// Display
		DisplayImage(m_trainImg[i], 3);

		// Localized Contour Sequence :
		trainImgLCSs[classIndex] = ExtractLCS();

		contourList.clear();

		// Training Class
		if (classIndex == m_classCount - 1)
		{
			// Statistical Dynamic Time Warping
			StatisticalDTW(trainImgLCSs, newPatt, deviPatt);

			// Store New Pattern
			m_newPatt.first = newPatt;
			m_newPatt.second = deviPatt;
			m_classPatt.push_back(m_newPatt);

			// Memory free
			for (int b = 0; b < m_classCount; b++)
			{
				free(trainImgLCSs[b]);
			}
		}
	}
	free(trainImgLCSs);
	// MFC
	GetDlgItem(IDC_TRAININGLOAD)->EnableWindow(FALSE);
	GetDlgItem(IDC_TRAININGLOAD)->ShowWindow(FALSE);
	GetDlgItem(IDC_TRAINING)->EnableWindow(FALSE);
	GetDlgItem(IDC_TESTLOAD)->EnableWindow(TRUE);
	GetDlgItem(IDC_TESTLOAD)->ShowWindow(TRUE);
}

void CRGBDlg::OnBnClickedTestload()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	static TCHAR BASED_CODE szFilter[] = _T("이미지 파일(*.BMP, *.GIF, *.JPG) | *.BMP;*.GIF;*.JPG;*.bmp;*.jpg;*.gif |모든파일(*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("*.jpg"), _T("image"), OFN_ALLOWMULTISELECT, szFilter);

	if (IDOK == dlg.DoModal())
	{
		for (POSITION pos = dlg.GetStartPosition(); pos != NULL;)
		{
			pathName = dlg.GetNextPathName(pos);
			CT2CA pszConvertedAnsiString_up(pathName);
			std::string up_pathName_str(pszConvertedAnsiString_up);
			m_testName.push_back(up_pathName_str);
			m_testImg[m_teSize++] = cv::imread(up_pathName_str);
		}

		GetDlgItem(IDC_SAVERESULT)->EnableWindow(TRUE);
	}
}

void CRGBDlg::OnBnClickedSaveresult()
{
	// Result
	double *imgLCS; // Image LCS temporary

	double *disSimilarity = (double *)calloc(m_classPatt.size() + 1, sizeof(double));

	int index;
	double minDisSim = 20150283;
	std::string::iterator iter;

	// Image Processing
	for (int i = 0; i < m_teSize; i++)
	{
		// GrayScale
		m_testImg[i] = Img2gray(m_testImg[i]);

		// BinaryImage
		m_testImg[i] = Img2otsu(m_testImg[i]);

		// Opening or Closing
		m_testImg[i] = Img2opening(m_testImg[i], kernel);
		//m_trainImg[i] = img2closing(m_trainImg[i], kernel);

		// Contour tracing
		m_testImg[i] = Img2contour(m_testImg[i]);

		// Display
		DisplayImage(m_testImg[i], 3);

		// Localized Contour Sequence : 가장 큰 Contour의 LCS
		imgLCS = ExtractLCS();
		contourList.clear();

		// Matching
		for (int j = 0; j < m_classPatt.size(); j++)
		{
			disSimilarity[j] = DTWarping(m_classPatt.at(j).first, imgLCS, m_classPatt.at(j).second);
		}
		index = 0;
		minDisSim = disSimilarity[0];
		for (int j = 1; j < m_classPatt.size(); j++)
		{
			if (minDisSim > disSimilarity[j])
			{
				minDisSim = disSimilarity[j];
				index = j; // 가장 비슷한 인덱스
			}
		}

		// Result
		printf("==============================================\n==============================================\n");

		iter = m_testName.at(i).end();
		while (iter != m_testName[i].begin())
		{
			iter--;
			if (*iter == (char)92)
			{
				iter++;
				break;
			}
		}
		while (iter != m_testName[i].end())
		{
			std::cout << *iter;
			iter++;
		}
		std::cout << " is ";
		index *= m_classCount;
		iter = m_className[index].end();
		while (iter != m_className[index].begin())
		{
			iter--;
			if (*iter == (char)92)
			{
				iter++;
				break;
			}
		}
		while (iter != m_className[index].end())
		{
			std::cout << *iter;
			iter++;
		}
		std::cout << std::endl;
	}

	// 초기화
	for (int i = 0; i < m_trSize; i++)
	{
		m_trainImg[i].release();
		m_trainImg[i] = NULL;
	}
	for (int i = 0; i < m_teSize; i++)
	{
		m_testImg[i].release();
		m_testImg[i] = NULL;
	}
	m_trSize = 0;
	m_teSize = 0;
	m_classPatt.clear();

	m_className.clear();
	m_testName.clear();

	// 초기화 후
	GetDlgItem(IDC_TRAININGLOAD)->EnableWindow(FALSE);
	GetDlgItem(IDC_TRAININGLOAD)->ShowWindow(TRUE);
	GetDlgItem(IDC_TESTLOAD)->EnableWindow(FALSE);
	GetDlgItem(IDC_TESTLOAD)->ShowWindow(FALSE);
	GetDlgItem(IDC_SAVERESULT)->EnableWindow(FALSE);
}

void CRGBDlg::DisplayImage(Mat targetMat, int channel)
{
	CDC *pDC = nullptr;
	CImage *mfcImg = nullptr;

	pDC = m_pic.GetDC();
	CStatic *staticSize = (CStatic *)GetDlgItem(IDC_Img);
	staticSize->GetClientRect(rect);

	cv::Mat tempImage;
	cv::resize(targetMat, tempImage, Size(rect.Width(), rect.Height()));

	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biBitCount = 24;
	bitmapInfo.bmiHeader.biWidth = tempImage.cols;
	bitmapInfo.bmiHeader.biHeight = tempImage.rows;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biClrImportant = 0;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 0;

	if (targetMat.channels() == 3)
	{
		mfcImg = new CImage();
		mfcImg->Create(tempImage.cols, tempImage.rows, 24);
	}
	else if (targetMat.channels() == 1)
	{
		cvtColor(tempImage, tempImage, COLOR_GRAY2RGB);
		mfcImg = new CImage();
		mfcImg->Create(tempImage.cols, tempImage.rows, 24);
	}
	else if (targetMat.channels() == 4)
	{
		bitmapInfo.bmiHeader.biBitCount = 32;
		mfcImg = new CImage();
		mfcImg->Create(tempImage.cols, tempImage.rows, 32);
	}
	cv::flip(tempImage, tempImage, 0);
	::StretchDIBits(mfcImg->GetDC(), 0, 0, tempImage.cols, tempImage.rows,
					0, 0, tempImage.cols, tempImage.rows, tempImage.data, &bitmapInfo,
					DIB_RGB_COLORS, SRCCOPY);

	mfcImg->BitBlt(::GetDC(m_pic.m_hWnd), 0, 0);

	if (mfcImg)
	{
		mfcImg->ReleaseDC();
		delete mfcImg; // mfcImg = nullptr;
	}
	tempImage.release();
	ReleaseDC(pDC);
}

void ChangeColor(Mat img, Mat &copy, int i)
{
	if (i == 1)
	{
		for (int y = 0; y < copy.rows; y++)
		{
			unsigned char *ptr1 = copy.data + 3 * (copy.cols * y);
			unsigned char *resultptr = copy.data + 3 * (copy.cols * y);
			for (int x = 0; x < copy.cols; x++)
			{
				// 이렇게 RGB값을 조정하여 그 범위 안에 있는 Rgb 픽셀값에 단색을 넣었다.
				//200 -> 160 -> 110
				//그림자
				//배경색을 초록으로 해도 결과가 바뀌므로 함부로 손대지 말자
				ptr1[3 * x + 0] = 0;
				ptr1[3 * x + 1] = 0;
				ptr1[3 * x + 2] = ptr1[3 * x + 2];
			}
		}
	}
	else if (i == 2)
	{
		for (int y = 0; y < copy.rows; y++)
		{
			unsigned char *ptr1 = copy.data + 3 * (copy.cols * y);
			unsigned char *resultptr = copy.data + 3 * (copy.cols * y);
			for (int x = 0; x < copy.cols; x++)
			{
				// 이렇게 RGB값을 조정하여 그 범위 안에 있는 Rgb 픽셀값에 단색을 넣었다.
				//200 -> 160 -> 110
				//그림자
				//배경색을 초록으로 해도 결과가 바뀌므로 함부로 손대지 말자
				ptr1[3 * x + 0] = 0;
				ptr1[3 * x + 1] = ptr1[3 * x + 1];
				ptr1[3 * x + 2] = 0;
			}
		}
	}
	else if (i == 3)
	{
		for (int y = 0; y < copy.rows; y++)
		{
			unsigned char *ptr1 = copy.data + 3 * (copy.cols * y);
			unsigned char *resultptr = copy.data + 3 * (copy.cols * y);
			for (int x = 0; x < copy.cols; x++)
			{
				// 이렇게 RGB값을 조정하여 그 범위 안에 있는 Rgb 픽셀값에 단색을 넣었다.
				//200 -> 160 -> 110
				//그림자
				//배경색을 초록으로 해도 결과가 바뀌므로 함부로 손대지 말자
				ptr1[3 * x + 0] = ptr1[3 * x + 0];
				ptr1[3 * x + 1] = 0;
				ptr1[3 * x + 2] = 0;
			}
		}
	}
}

Mat Img2red(Mat img)
{
	Mat red = img.clone();
	ChangeColor(img, red, 1);

	return red;
}

Mat Img2green(Mat img)
{
	Mat green = img.clone();
	ChangeColor(img, green, 2);

	return green;
}

Mat Img2blue(Mat img)
{
	Mat blue = img.clone();
	ChangeColor(img, blue, 3);

	return blue;
}

Mat Img2gray(Mat img)
{
	Mat tmpImg = img.clone();
	int gRows = tmpImg.rows;
	int gCols = tmpImg.cols;
	Mat grayScale(gRows, gCols, CV_8UC1); // 1channel

	double r, g, b, gray;

	for (int j = 0; j < gRows; j++)
	{
		for (int i = 0; i < gCols; i++)
		{
			b = tmpImg.at<Vec3b>(j, i)[0];
			g = tmpImg.at<Vec3b>(j, i)[1];
			r = tmpImg.at<Vec3b>(j, i)[2];

			gray = r * 0.2126 + g * 0.7152 + b * 0.0722;

			grayScale.at<uchar>(j, i) = (uchar)gray;
		}
	}

	tmpImg.release();

	return grayScale;
}

Mat Img2otsu(Mat img)
{
	Mat otsu = img.clone();
	int oRows = otsu.rows;
	int oCols = otsu.cols;

	double tMean = 0.0; // totalMean
	int idt[256] = {
		0,
	}; // Histogram
	double pIdt[256] = {
		0.0,
	}; // N-histogram

	double wgt_0, wgt_1;   // weight-0, weight-1
	double mean_0, mean_1; // mean-0, mean-1

	double result = 0.0; // between-class deviation

	// Histogram
	for (int j = 0; j < oRows; j++)
	{
		for (int i = 0; i < oCols; i++)
		{
			idt[(int)otsu.at<uchar>(j, i)]++;
		}
	}

	// N-Histogram + Total-Mean
	for (int i = 0; i < 256; i++)
	{
		pIdt[i] = idt[i] / ((double)oRows * oCols);
		tMean += pIdt[i] * i;
	}

	// otsu's thresholding :: i = threshold
	for (int i = 0; i < 256; i++)
	{
		// weight initialize
		wgt_0 = 0.0;
		mean_0 = 0.0;
		for (int j = 0; j <= i; j++)
		{
			wgt_0 += pIdt[j];
			mean_0 += pIdt[j] * j;
		}
		wgt_1 = 1.0 - wgt_0;
		mean_1 = tMean - mean_0;

		// mean initialize
		if (wgt_0 != 0)
		{
			mean_0 /= wgt_0;
		}
		if (wgt_1 != 0)
		{
			mean_1 /= wgt_1;
		}

		double next_result = (wgt_0 * (mean_0 - tMean) * (mean_0 - tMean)) + (wgt_1 * (mean_1 - tMean) * (mean_1 - tMean));

		if (result <= next_result)
		{ // continue
			result = next_result;
		}
		else
		{ // find threshold :: i-1(maximum result) + break
			for (int b = 0; b < oRows; b++)
			{
				for (int a = 0; a < oCols; a++)
				{
					if (otsu.at<uchar>(b, a) < i)
					{ // i-1
						otsu.at<uchar>(b, a) = 0;
					}
					else
					{
						otsu.at<uchar>(b, a) = 255;
					}
				}
			}
			break; // exit first for-loop
		}
	}

	return otsu;
}

Mat Img2dilation(Mat img, Mat kernel)
{
	int dRows = img.rows; // img rows size
	int dCols = img.cols; // img cols size
	Mat dilImg = Mat::zeros(dRows, dCols, CV_8UC1);

	int kRows = kernel.rows;   // kernel rows size
	int kCols = kernel.cols;   // kernel cols size
	int kMidIndex = kRows / 2; // kernel mid row index

	int tmpJindex = 0, tmpIindex = 0;

	uchar check = 0;

	for (int j = 0; j < dRows; j++)
	{
		for (int i = 0; i < dCols; i++)
		{
			for (int b = 0; b < kRows; b++)
			{
				for (int a = 0; a < kCols; a++)
				{
					tmpJindex = j + (b - kMidIndex);
					tmpIindex = i + (a - kMidIndex);
					if (tmpJindex >= 0 && tmpJindex < dRows && tmpIindex >= 0 && tmpIindex < dCols)
					{
						if (kernel.at<uchar>(b, a) == 255)
						{
							check |= img.at<uchar>(tmpJindex, tmpIindex);
						}
					}
				}
			}
			dilImg.at<uchar>(j, i) = check; // dilation
			check = 0;
		}
	}

	return dilImg;
}

Mat Img2erosion(Mat img, Mat kernel)
{
	int eRows = img.rows; // img rows size
	int eCols = img.cols; // img cols size
	Mat eroImg = Mat::zeros(eRows, eCols, CV_8UC1);

	int kRows = kernel.rows;   // kernel rows size
	int kCols = kernel.cols;   // kernel cols size
	int kMidIndex = kRows / 2; // kernel mid row index

	int tmpJindex = 0, tmpIindex = 0;

	uchar check = 255;

	for (int j = 0; j < eRows; j++)
	{
		for (int i = 0; i < eCols; i++)
		{
			for (int b = 0; b < kRows; b++)
			{
				for (int a = 0; a < kCols; a++)
				{
					tmpJindex = j + (b - kMidIndex);
					tmpIindex = i + (a - kMidIndex);
					if (tmpJindex >= 0 && tmpJindex < eRows && tmpIindex >= 0 && tmpIindex < eCols)
					{
						if (kernel.at<uchar>(b, a) == 255)
						{
							check &= img.at<uchar>(tmpJindex, tmpIindex);
						}
					}
				}
			}
			eroImg.at<uchar>(j, i) = check; // erosion
			check = 255;
		}
	}

	return eroImg;
}

Mat Img2opening(Mat img, Mat kernel)
{
	Mat openImg;

	openImg = Img2erosion(img, kernel);
	openImg = Img2dilation(openImg, kernel);

	return openImg;
}

Mat Img2closing(Mat img, Mat kernel)
{
	Mat closeImg;

	closeImg = Img2dilation(img, kernel);
	closeImg = Img2erosion(closeImg, kernel);

	return closeImg;
}

Mat Img2contour(Mat img)
{
	int cRows = img.rows;
	int cCols = img.cols;

	Mat originImg = img.clone();
	Mat labelImg;

	for (int j = 0; j < cRows; j++)
	{
		originImg.at<uchar>(j, 0) = 0;
		originImg.at<uchar>(j, cCols - 1) = 0;
	}

	for (int i = 0; i < cCols; i++)
	{
		originImg.at<uchar>(0, i) = 0;
		originImg.at<uchar>(cRows - 1, i) = 0;
	}

	// Display
	Mat contourImg = Mat::zeros(cRows, cCols, CV_8UC1); // Contour Display 하기 위한 이미지
	labelImg = LabelingwithBT(originImg, contourImg);	// labelImg : 라벨링 결과 이미지
	return contourImg;									// 출력하기 위한 Contour 이미지
}

Mat LabelingwithBT(Mat binaryImg, Mat &contourImg)
{
	int bRows = binaryImg.rows;
	int bCols = binaryImg.cols;

	int **LUT_BLabeling = InitLUT(8);

	Mat labelImg = Mat::zeros(bRows, bCols, CV_8UC1);

	int FOREWARD = 20150;
	int BACKWARD = 283;

	uchar cur_p;
	int ref_p1, ref_p2;

	int labelnumber = 1;

	for (int j = 1; j < (bRows - 1); j++)
	{
		for (int i = 1; i < (bCols - 1); i++)
		{
			cur_p = binaryImg.at<uchar>(j, i);
			if (cur_p == 255 && labelImg.at<uchar>(j, i) == 0)
			{ // object
				ref_p1 = labelImg.at<uchar>(j, i - 1);
				ref_p2 = labelImg.at<uchar>(j - 1, i - 1);
				if (ref_p1 > 1)
				{ // propagation
					labelImg.at<uchar>(j, i) = ref_p1;
				}
				else if (ref_p1 == 0 && ref_p2 >= 2)
				{ // hole
					labelImg.at<uchar>(j, i) = ref_p2;
					BTracing8(j, i, ref_p2, BACKWARD, binaryImg, labelImg, contourImg, LUT_BLabeling);
				}
				else if (ref_p1 == 0 && ref_p2 == 0)
				{ // region start
					labelImg.at<uchar>(j, i) = ++labelnumber;
					BTracing8(j, i, labelnumber, FOREWARD, binaryImg, labelImg, contourImg, LUT_BLabeling);
				}
			}
		}
	}

	// free Look up table
	for (int j = 0; j < 8; j++)
	{
		free(LUT_BLabeling[j]);
	}
	free(LUT_BLabeling);

	return labelImg;
}

void BTracing8(int y, int x, int label, int tag, Mat binImg, Mat &labImg, Mat &boundImg, int **LUT_BLabeling)
{
	int cur_orient, pre_orient;

	int end_x, _x;
	int end_y, _y;

	if (tag == 20150)
	{
		cur_orient = pre_orient = 0;
	} // FOREWARD
	else
	{
		cur_orient = pre_orient = 6;
	} // BACKWARD

	end_x = _x = x;
	end_y = _y = y;

	int *neighbor8 = (int *)calloc(8, sizeof(int));

	int start_o, add_o;
	int i;

	do
	{
		// Display(Gray Color)
		boundImg.at<uchar>(y, x) = 127;
		// for LCSs
		_xy.first = y;
		_xy.second = x;
		_xyList.push_back(_xy);

		Read_neighbor8(y, x, neighbor8, binImg);
		start_o = (8 + cur_orient - 2) % 8;
		for (i = 0; i < 8; i++)
		{
			add_o = (start_o + i) % 8;
			if (neighbor8[add_o] == 255)
			{
				break;
			}
		}
		if (i < 8)
		{
			CalcOrd(add_o, &y, &x);
			cur_orient = add_o;
		}
		if (LUT_BLabeling[pre_orient][cur_orient] == 1)
		{
			labImg.at<uchar>(_y, _x) = label;
		}
		_y = y;
		_x = x;
		pre_orient = cur_orient;
	} while ((y != end_y) || (x != end_x));

	free(neighbor8);
	if (!contourList.empty())
	{
		if (_xyList.size() > contourList.front().size())
		{
			contourList.insert(contourList.begin(), _xyList);
		}
	}
	else
	{
		if (_xyList.at(0).first != 1 && _xyList.at(0).second != 1)
		{
			contourList.push_back(_xyList);
		}
	}
	_xyList.clear();
}

void CalcOrd(int i, int *y, int *x)
{
	switch (i)
	{
	case 0:
		*x = *x + 1;
		break;
	case 1:
		*y = *y + 1;
		*x = *x + 1;
		break;
	case 2:
		*y = *y + 1;
		break;
	case 3:
		*y = *y + 1;
		*x = *x - 1;
		break;
	case 4:
		*x = *x - 1;
		break;
	case 5:
		*y = *y - 1;
		*x = *x - 1;
		break;
	case 6:
		*y = *y - 1;
		break;
	case 7:
		*y = *y - 1;
		*x = *x + 1;
		break;
	}
}

void Read_neighbor8(int y, int x, int *neighbor8, Mat img)
{
	neighbor8[0] = (int)img.at<uchar>(y, x + 1);
	neighbor8[1] = (int)img.at<uchar>(y + 1, x + 1);
	neighbor8[2] = (int)img.at<uchar>(y + 1, x);
	neighbor8[3] = (int)img.at<uchar>(y + 1, x - 1);
	neighbor8[4] = (int)img.at<uchar>(y, x - 1);
	neighbor8[5] = (int)img.at<uchar>(y - 1, x - 1);
	neighbor8[6] = (int)img.at<uchar>(y - 1, x);
	neighbor8[7] = (int)img.at<uchar>(y - 1, x + 1);
}

int **InitLUT(int size)
{
	// Allocate memory
	int **LUT_BLabeling = (int **)calloc(size, sizeof(int *));
	for (int i = 0; i < size; i++)
	{
		LUT_BLabeling[i] = (int *)calloc(size, sizeof(int));
		for (int j = 0; j < size; j++)
		{
			LUT_BLabeling[i][j] = 0;
		}
	}

	// Init Look up table
	if (size == 4)
	{
		LUT_BLabeling[1][3] = 1;
		LUT_BLabeling[2][0] = 1;
		LUT_BLabeling[2][3] = 1;
		LUT_BLabeling[3][0] = 1;
		LUT_BLabeling[3][1] = 1;
		LUT_BLabeling[3][3] = 1;
	}
	else
	{
		for (int i = 4; i < 8; i++)
		{
			for (int j = 0; j < i - 3; j++)
			{
				LUT_BLabeling[i][j] = 1;
			}
		}
		for (int i = 1; i < 4; i++)
		{
			for (int j = 5; j < 5 + i; j++)
			{
				LUT_BLabeling[i][j] = 1;
			}
		}
		for (int i = 4; i < 8; i++)
		{
			for (int j = 5; j < 8; j++)
			{
				LUT_BLabeling[i][j] = 1;
			}
		}
	}

	return LUT_BLabeling;
}

double *ExtractLCS()
{
	unsigned int totalSize = 0;

	int window;
	int tempWindow;
	// Memory 할당 및 수직 유클리디안 거리 계산

	for (int i = 0; i < contourList.size(); i++)
	{
		totalSize += contourList.at(i).size();
	}

	double *LCS = (double *)calloc(totalSize, sizeof(double));

	int lcsIndex = 0;

	// h(i) 구하기
	for (int i = 0; i < contourList.size(); i++)
	{
		if (contourList.at(i).size() > 8)
		{
			tempWindow = contourList.at(i).size() / 8;
		}
		else
		{
			tempWindow = 1;
		}
		window = (tempWindow % 2 == 0) ? tempWindow + 1 : tempWindow; // window size =  Contour size / 8의 홀수
		for (int j = 0; j < contourList.at(i).size(); j++)
		{
			LCS[lcsIndex++] = FindDistance(i, j, window);
		}
	}

	return LCS;
}

double FindDistance(int contourIndex, int pairIndex, int window)
{
	double dist;

	int iterator = window / 2; // int = (window - 1) / 2
	int size = contourList.at(contourIndex).size();

	int _y = contourList.at(contourIndex).at(pairIndex).first;	// hx
	int _x = contourList.at(contourIndex).at(pairIndex).second; // hy
	int pIndex, _yPlus, _xPlus;									// h(y, x) + (window - 1) / 2
	int mIndex, _yMinus, _xMinus;								// h(y, x) - (window - 1) / 2

	pIndex = (pairIndex + iterator) % size;

	mIndex = (pairIndex - iterator);
	if (mIndex < 0)
	{
		mIndex += size;
	}

	_yPlus = contourList.at(contourIndex).at(pIndex).first;
	_xPlus = contourList.at(contourIndex).at(pIndex).second;
	_yMinus = contourList.at(contourIndex).at(mIndex).first;
	_xMinus = contourList.at(contourIndex).at(mIndex).second;

	dist = fabs(_y * (_xMinus - _xPlus) + _x * (_yPlus - _yMinus) + (_xPlus * _yMinus) - (_yPlus * _xMinus)) / sqrt((_xMinus - _xPlus) * (_xMinus - _xPlus) + (_yPlus - _yMinus) * (_yPlus - _yMinus));

	return dist;
}

double DTWarping(double *LCS1, double *LCS2, double *deviPatt)
{
	// Dissimilarity --> return
	double disS;

	// LCS1 : size1 ------ LCS2 : size2
	int size1 = _msize(LCS1) / sizeof(LCS1[0]);
	int size2 = _msize(LCS2) / sizeof(LCS2[0]);

	const double inf = 20150283;

	// D, G Matrix
	double **dMat;
	int **gMat;

	// Matrix Memory 할당 --> Mem[size1][size2]								(j)		size2 (LCS2)
	dMat = (double **)calloc(size1, sizeof(double *)); //	s(i)I----------------------
	gMat = (int **)calloc(size1, sizeof(int *));	   //	i	I
	for (int i = 0; i < size1; i++)
	{													   //	z	I
		dMat[i] = (double *)calloc(size2, sizeof(double)); //	e	I
		gMat[i] = (int *)calloc(size2, sizeof(int));	   //	1	I
	}													   // (LCS1)

	// Initialization
	dMat[0][0] = fabs(LCS1[0] - LCS2[0]);
	if (deviPatt && deviPatt[0])
	{
		dMat[0][0] /= deviPatt[0];
	}
	gMat[0][0] = 0;

	for (int j = 1; j < size2; j++)
	{ // j축(가로 축) : 열
		dMat[0][j] = fabs(LCS1[0] - LCS2[j]);
		if (deviPatt && deviPatt[0])
		{
			dMat[0][j] /= deviPatt[0];
		}
		dMat[0][j] += dMat[0][j - 1];
		gMat[0][j] = 2;
	}
	for (int i = 1; i < size1; i++)
	{ // i축(세로 축) : 행
		dMat[i][0] = inf;
	}

	// Forward
	for (int i = 1; i < size1; i++)
	{
		for (int j = 1; j < size2; j++)
		{
			dMat[i][j] = fabs(LCS1[i] - LCS2[j]);
			if (deviPatt && deviPatt[i])
			{
				dMat[i][j] /= deviPatt[i];
			}
			gMat[i][j] = ForDTW_MinArg(i, j, dMat);
		}
	}

	// Backtracking
	int i = size1 - 1;
	int j = size2 - 1;
	int k = 0;

	while (i != 0 && j != 0)
	{
		_path.push_back(gMat[i][j]);
		switch (gMat[i][j])
		{
		case 1:
			i--;
			k++;
			break;
		case 2:
			j--;
			k++;
			break;
		case 3:
			i--;
			j--;
			k++;
			break;
		}
	}

	// Termination
	disS = dMat[size1 - 1][size2 - 1] / ((double)k + 1);

	_imgPath.push_back(_path);
	_path.clear();

	// Memory free
	for (int f = 0; f < size1; f++)
	{
		free(dMat[f]);
		free(gMat[f]);
	}
	free(dMat);
	free(gMat);

	return disS;
}

int ForDTW_MinArg(int i, int j, double **&dMat)
{
	int arg = 1; // 1 or 2 or 3
	double min = dMat[i - 1][j];

	if (dMat[i][j - 1] < min)
	{
		arg = 2;
		min = dMat[i][j - 1];
	}
	if (dMat[i - 1][j - 1] < min)
	{
		arg = 3;
		min = dMat[i - 1][j - 1];
	}
	dMat[i][j] += min;

	return arg;
}

void StatisticalDTW(double **trainImgLCSs, double *&newPatt, double *&deviPatt)
{
	thresArray.clear();
	thresArray.push_back(0);
	bool isChange = true;
	int imgCnt = _msize(trainImgLCSs) / sizeof(trainImgLCSs[0]); // trainImg 개수

	// 가장 작은 LCS Size가 기준
	int standardIndex = 0;
	int pattCnt = _msize(trainImgLCSs[0]) / sizeof(trainImgLCSs[0][0]);
	int temp;
	for (int i = 1; i < imgCnt; i++)
	{
		temp = _msize(trainImgLCSs[i]) / sizeof(trainImgLCSs[i][0]);
		if (pattCnt > temp)
		{
			pattCnt = temp;
			standardIndex = i;
		}
	}

	// Allocate Memory
	newPatt = (double *)calloc(pattCnt, sizeof(double));		 // 최종 평균 패턴
	deviPatt = (double *)calloc(pattCnt, sizeof(double));		 // 최종 표준 편차 패턴
	double *curPatt = (double *)calloc(pattCnt, sizeof(double)); // 새로운 평균 패턴
	double *nullPtr = nullptr;

	// Initialize : 처음 평균 패턴은 가장 작은 LCS Size의 이미지
	for (int i = 0; i < pattCnt; i++)
	{
		newPatt[i] = curPatt[i] = trainImgLCSs[standardIndex][i];
	}

	// Statistical Dynamic Time Warping
	while (isChange)
	{
		for (int i = 0; i < imgCnt; i++)
		{
			DTWarping(curPatt, trainImgLCSs[i], nullPtr);
		}
		ForSDTW_Average(trainImgLCSs, curPatt); // 새로운 평균 패턴 생성 + 매칭 위치별로 분산

		if (FindThres(curPatt, newPatt) < 1)
		{
			isChange = false;
		} // 평균 패턴이 변하는지 검사
		else
		{
			_imgPath.clear();
		}

		for (int a = 0; a < pattCnt; a++)
		{
			newPatt[a] = curPatt[a]; // 새로운 평균 패턴
		}
	}
	ForSDTW_Deviation(trainImgLCSs, newPatt, deviPatt); // 분산

	// Memory free
	free(curPatt);
	_imgPath.clear();
}

void ForSDTW_Average(double **trainImgLCSs, double *&avgPatt)
{
	int imgCnt = _msize(trainImgLCSs) / sizeof(trainImgLCSs[0]);
	int pattCnt = _msize(avgPatt) / sizeof(avgPatt[0]);

	int *matchCnt = (int *)calloc(pattCnt, sizeof(int));
	int mIndex;

	for (int i = 0, j = 0; i < imgCnt; i++)
	{
		avgPatt[0] += trainImgLCSs[i][0];
		matchCnt[0]++;
		mIndex = 0;
		for (int k = _imgPath.at(i).size() - 1; k >= 0; k--)
		{ // 이미지당 path size만큼
			switch (_imgPath.at(i).at(k))
			{
			case 1:
				j++;
				matchCnt[j]++;
				avgPatt[j] += trainImgLCSs[i][mIndex];
				break;
			case 2:
				mIndex++;
				matchCnt[j]++;
				avgPatt[j] += trainImgLCSs[i][mIndex];
				break;
			case 3:
				j++;
				mIndex++;
				matchCnt[j]++;
				avgPatt[j] += trainImgLCSs[i][mIndex];
				break;
			}
		}
		j = 0;
	}

	for (int j = 0; j < pattCnt; j++)
	{
		avgPatt[j] /= ((double)matchCnt[j] + 1);
	}

	free(matchCnt);
}

void ForSDTW_Deviation(double **trainImgLCSs, double *avgPatt, double *&deviPatt)
{
	int imgCnt = _msize(trainImgLCSs) / sizeof(trainImgLCSs[0]);
	int pattCnt = _msize(deviPatt) / sizeof(deviPatt[0]);

	int *matchCnt = (int *)calloc(pattCnt, sizeof(int));
	int mIndex;

	for (int i = 0, j = 0; i < imgCnt; i++)
	{
		deviPatt[0] += (avgPatt[0] - trainImgLCSs[i][0]) * (avgPatt[0] - trainImgLCSs[i][0]);
		matchCnt[0]++;
		mIndex = 0;
		for (int k = _imgPath.at(i).size() - 1; k >= 0; k--)
		{ // 이미지당 path size만큼
			switch (_imgPath.at(i).at(k))
			{
			case 1:
				j++;
				matchCnt[j]++;
				deviPatt[j] += (avgPatt[j] - trainImgLCSs[i][mIndex]) * (avgPatt[j] - trainImgLCSs[i][mIndex]);
				break;
			case 2:
				mIndex++;
				matchCnt[j]++;
				deviPatt[j] += (avgPatt[j] - trainImgLCSs[i][mIndex]) * (avgPatt[j] - trainImgLCSs[i][mIndex]);
				break;
			case 3:
				j++;
				mIndex++;
				matchCnt[j]++;
				deviPatt[j] += (avgPatt[j] - trainImgLCSs[i][mIndex]) * (avgPatt[j] - trainImgLCSs[i][mIndex]);
				break;
			}
		}
		j = 0;
	}

	for (int j = 0; j < pattCnt; j++)
	{
		deviPatt[j] = sqrt(deviPatt[j] / (double)matchCnt[j]);
	}

	free(matchCnt);
}

double FindThres(double *curPatt, double *prePatt)
{
	double thres = 0;
	int pattCnt = _msize(prePatt) / sizeof(prePatt);

	for (int i = 0; i < pattCnt; i++)
	{
		thres += fabs(curPatt[i] - prePatt[i]);
	}
	thresArray.insert(thresArray.begin(), thres);

	printf("-----------------Threshold = %lf--------------\n", thres);

	if (thresArray.size() > 100)
	{
		sortThres.assign(thresArray.begin(), thresArray.end());
		sort(sortThres.begin(), sortThres.end());
		if (sortThres.back() / sortThres.front() < 2 || sortThres.at(0) == sortThres.at(1) || sortThres.at(8) == sortThres.at(9))
		{
			return 0.0;
		}
		sortThres.clear();
		thresArray.pop_back();
	}

	return thres;
}