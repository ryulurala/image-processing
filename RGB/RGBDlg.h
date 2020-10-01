#include <opencv2/opencv.hpp>
#include "afxwin.h"
#include <utility>
#include <vector>
#include <string>
#include <algorithm>

using namespace cv;
// RGBDlg.h : 헤더 파일
//

#pragma once


// CRGBDlg 대화 상자
class CRGBDlg : public CDialogEx
{
	// 생성입니다.
public:
	CRGBDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	
	Mat m_trainImg[100];
	std::vector<std::string> m_className;
	int m_trSize;
	int m_classCount;

	Mat m_testImg[100];
	std::vector<std::string> m_testName;
	int m_teSize;

	std::pair<double*, double*> m_newPatt;
	std::vector<std::pair<double*, double*>> m_classPatt;

	CString pathName;
	CRect rect;

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RGB_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_pic;
	//이미지 load용
	void CRGBDlg::DisplayImage(Mat targetMat, int channel);

	afx_msg void OnBnClickedTrainingload();
	afx_msg void OnBnClickedTestload();
	afx_msg void OnBnClickedSaveresult();
	afx_msg void OnBnClickedTraining();
};
