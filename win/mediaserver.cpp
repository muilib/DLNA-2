// mediaserver.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "DlnaProxy.h"


int _tmain(int argc, _TCHAR* argv[])
{
	char buf[512];
	CDlnaProxy* pDlna = CDlnaProxy::GetInstance();
	pDlna->SetControlPoint();
	
	while (gets_s(buf))
	{
		if (strncmp("a", buf, 1) == 0)
		{
			pDlna->SetMediaServer();
		}
		else if (strncmp("e", buf, 1) == 0)
		{
			pDlna->SetMediaServer(false);
		}
		else if (strncmp("q", buf, 1) == 0)
		{
			printf("Quit.....\n");
			break;
		}
	}

	pDlna->Stop();

	return 0;
}

