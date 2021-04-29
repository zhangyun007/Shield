#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

#include "../TinyTL/TObject.hpp"

//���ں���SetConsoleOutputCP(65001);����cmd����Ϊutf8

using namespace Gdiplus;
using namespace std;

#pragma comment(lib,"User32.lib")
#pragma comment(lib,"ComCtl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "wsock32.lib")

#define IDT_TIMER1 12

#define RED_COLOR RGB(255,0,0)
#define BLUE_COLOR RGB(0,0,255)

// �ַ�������
vector<string> Val;

//�洢@var���ֵı�������ֵVal���±ꡣ
map<string, string> var;

int cxScreen,cyScreen;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//ȥ��line��ͷ�Ŀհ��ַ���
string Skip_Blank(string line) {
	int i;
  for (i=0; i<line.length(); i++) {
    if (line[i]!=' ' && line[i]!='\t')
      break;
  }
	if (i<line.length()-i)
		return line.substr(i, line.length()-i);
	else
		return "";
}

//�õ�һ���ı���ͷһ�����ʡ�
string Get_First(string line) {
  string s = "";
  for (int i=0; i<line.length(); i++) {
    if (line[i] != ' ' && line[i] != '\t' && i < line.length())
      s += line[i];
    else
      break;
  }
  return s;
}

//�ж�ĳ�ַ����Ƿ�������
bool Is_Int(string s) {
  for (auto i:s) {
    if (isdigit(i) == false)
      return false;
  }
  return true;
}

//�ж�s�Ƿ���Element�������ã��ú���Ϊ���ú��������ڷ���ĳ���ڵ㡣
bool Is_Fun_Element(string s) {
  return true;
}

//���һ�����ӵ�Զ�̷�����
string lasthost = "";

string Get_Host(string s) {
  string tmp;
  for (int i=0; i<s.length(); i++) {
    if (s[i]=='@') {
      tmp=s.substr(i+1);
      if (tmp == "_VAL") {
       i = atoi(s.substr(i+4).c_str());
        lasthost = Val[i];
        return lasthost;
      } else {
        lasthost = var[tmp];
        return lasthost;
      }
    }
  }
  return lasthost;
}

//ȡ�ú�����
string Get_Fun_Call(string s) {
  for (int i = 0; i < s.length(); i++) {
    if (s[i] == '@') {
      return s.substr(0, i);
    }
  }
  return s;
}

//���s�Ƿ����ch
bool HasChar(string s, char ch) {
  for (int i=0; i<=s.length(); i++) {
    if (s[i] == ch)
      return true;
  }
  return false;
}


class Connection {
  public:
    string Host;
    SOCKET sock;
    Connection(string s);
    ~Connection();
    //���÷�����Զ�̺������õ�����ֵ
    string RPC(string s);
};

vector<class Connection*> vc;

//���캯��
Connection::Connection(string h) {
  Host = h;
  
  //��ʼ��DLL
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  //�����׽���
  SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  //���������������
  sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));  //ÿ���ֽڶ���0���
  sockAddr.sin_family = PF_INET;
  sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  sockAddr.sin_port = htons(9999);
  connect(sock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
}

Connection::~Connection() {
  //�ر��׽���
  closesocket(sock);
  //��ֹʹ�� DLL
  WSACleanup();
}

string Connection::RPC(string s) {
  //���շ��������ص�����
  char szBuffer[64] = {0};
  send(sock, s.c_str(), s.length(), 0);
  recv(sock, szBuffer, 64, NULL);
  //������յ�������
  printf("Message form server: %s\n", szBuffer);
  return szBuffer;
}


// ����S��ע�⣬��������ȼ���
string Evaluate(string f) {	  
  for (int i=0; i<f.length(); i++) {
    //��ֵ���
    if (f[i]=='=') {
      string v = f.substr(0, i);
      
      //�б�����ֵ�Ԫ�ظ�ֵ
      if (HasChar(v, '|') == true) {
        break;
      }
      
      //��$��ͷ����ʾΪ�ڵ����Ը�ֵ
      if (v[0]=='$') {
        break;
      }
      
      for (auto j: var) {
        if (j.first == v) {
          //������ֵ
          var[v] = Evaluate(f.substr(i+1, f.length()));
          break;
        }
      }
      
      break;
    }
  }
  
  //�ڵ�Insert Delete 
  if (f[0]=='$') {
  
  }
  
  //�õ�Զ�̷�������
  string host = Get_Host(f);
  
  //�õ��������Ͳ���
  string n = Get_Fun_Call(f);
  
  for (auto i:vc) {
    if (i->Host==host) {
      return i->RPC(n);
    } else {
      class Connection *con = new Connection(host);
      vc.push_back(con);
      return con->RPC(n);
    }
  }
  
  return "";
}


class Proc {
public:
  string name;    //��������������"@init" "" ...
  vector<string> s; //�ѹ��̵������Կո����֣����vector<string> s
  void Call();
};

void Proc::Call(){
	cout << "calling .." << name << "\n";
	for (auto i: s) {
		Evaluate(i);
	}
}

//�����б�
vector<class Proc *> vp;


//����������-1��ʾû�ҵ���
int InVal(string s) {
	for (int i=0; i<Val.size(); i++) {
		if (Val[i] == s) {
			return i;
		}
	}
	return -1;
}

/*���ַ����洢��vector<string> val�У����ö�Ӧ��vector�±����滻�ַ�����
  ��ô����Ŀ����Ϊ�˷������ʷ��������Կո�Ϊ�����*/
string String_2_Int(string line){
  int i = 0;
  string str = "";
  short n=0;
   
  while (1) {
    string s = "";
    
    while (line[i] != '\"') {
      str+=line[i];
      if (i == (line.length()-1))
        return str;
      i++;
    }
    
    //�˴�����⵽��һ����
    i++;
    
    while (line[i] != '\"') {
      s+=line[i];
      if (i == (line.length()-1)) {
        cout << " �����ɶԡ�\n";
        exit(0);
      }
      i++;
    }
    
    //�˴����ڶ���"����⵽
    int m = InVal(s);
    
    //����������   
    char buf[10];
    
    if (m >= 0 ) {
      sprintf(buf,"%d",m);
      str=str+"_VAL"+buf;				
    } else {
      Val.push_back(s);
      n = Val.size();		
      sprintf(buf,"%d",n-1);
      str=str+"_VAL"+buf;		
    }
    
    if (i == (line.length()-1))
      break;
    i++;
  }
  return str;
}

/* ָ���ṹ��һ��ָ�����ʽ���£�

#LOOP i=0 i+1 i<4
.....
END

#IF i>12
.....
END

#DEF S1
.....
END

һ��ָ�������ܰ���N��ָ���

#LOOP i=0 i+1 i<4

#IF i>12
.....
END
.....

END

*/

class Instruction_Block {
	char * instruction;		//������һ�У�Ҳ����ָ����  ��#LOOP i=0 i+1 i<4��
};

class GUI_Element {
public:  
  
  unsigned short Level;         //�ؼ��ڵڼ�Level��
  string Name;                  //�ؼ����� �����ܵ�ValΪ"<window>" "<text>" "<div>" ...
  map<string, string> Property; //�ؼ�����  titile=1, 1ΪVal��index.
  
  //�ؼ���������  ��λ�Ƿֱ���  left top right bottom
  int l;
  int t;
  int r;
  int b;
  
  class GUI_Element *parent;    //���ڵ�
  class GUI_Element *child;     //��һ���ӽڵ�
  class GUI_Element *brother;   //��һ���ֵܽڵ�

  GUI_Element(string line);  
  
};

//line���ļ��е�һ�У�1 window title="��һ�� ����"
GUI_Element::GUI_Element(string line) {  
  string s = Get_First(line);
                            
  for (int i=0; i<s.length(); i++){
    if (!isdigit(s[i])) {
      cout << "You should start with Level number.\n";
      return;
    }
  }
  
  Level = atoi(s.c_str());
  
  string tmp = Skip_Blank(line.substr(s.length()));
  Name = Get_First(tmp);
  
  tmp = Skip_Blank(tmp.substr(Name.length()));
  string f = Get_First(tmp);
  //�������һ������
  while (f != tmp) {
    for (int i=0; i<f.length(); i++) {
      if (f[i]=='=') {
        //cout << f.substr(i+1, f.length()) << "\n";
        Property[f.substr(0, i)] = f.substr(i+1, f.length());
        break;
      }
    }
    tmp = Skip_Blank(tmp.substr(f.length()));
    f = Get_First(tmp);
  }
  for (int i=0; i<f.length(); i++) {
    if (f[i]=='=') {
      Property[f.substr(0, i)] = f.substr(i+1, f.length());
    }
  }
  cout << Level << ", "<< Name << "\n";
  for(auto iter = Property.begin(); iter != Property.end(); iter++) {
    cout << iter->first << " : " << iter->second << endl;
  }
}

struct Window_Element {
  HWND  hwnd;
  class GUI_Element *head;
  void Draw_Window();
};

//�����б�
vector<struct Window_Element *> v_window;

//����Window
void Window_Element::Draw_Window() {
  
  class GUI_Element *tmp = this->head;
  
  //���ƶ��㴰��
  if (tmp->Name == "WINDOW") {
    MSG         msg;
    string      title = "Test Window";
    
    int x = atof(tmp->Property["left"].c_str()) * cxScreen;
    int y = atof(tmp->Property["top"].c_str()) * cyScreen;
    int w = (atof(tmp->Property["right"].c_str()) - atof(tmp->Property["left"].c_str())) * cxScreen;
    int h = (atof(tmp->Property["bottom"].c_str()) - atof(tmp->Property["top"].c_str())) * cyScreen;
    
    this->hwnd = CreateWindow(
      TEXT("MyClass"),   			  // window class name
      TEXT(title.c_str()),  		        
      WS_OVERLAPPEDWINDOW,     	// window style
      x,           	// initial x position
      y,           	// initial y position
      w,           	// initial x size
      h,           	// initial y size
      NULL,						            // parent window handle
      NULL,                     	// window menu handle
      NULL,          			  	    // program instance handle
      NULL);                    	// creation parameters			
      
    /*
    SetTimer(w.hwnd,             // handle to main window 
      IDT_TIMER1,            // timer identifier 
      1000,                 // 10-second interval 
      (TIMERPROC) NULL);     // no timer callback 

    SetWindowText(w.hwnd, "aaa");    */
  
    ShowWindow(this->hwnd, 1);
    UpdateWindow(this->hwnd);
		
	//����һ��WM_PAINTER��Ϣ�������ӿؼ���
    //InvalidateRect(this->hwnd, NULL, TRUE);

    //��Ϣѭ��
    while(GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

void DrawBezier(HDC hdc, POINT apt[]) {
    PolyBezier(hdc, apt, 4);

    MoveToEx(hdc, apt[0].x, apt[0].y, NULL);
    LineTo(hdc, apt[1].x, apt[1].y);

    MoveToEx(hdc, apt[2].x, apt[2].y, NULL);
    LineTo(hdc, apt[3].x, apt[3].y);
}

//����Window������ӿؼ���ͬһ�㣬����ƵĿ��ܻḲ���Ȼ��Ƶ�;  �ȸ����ӣ����ֺ�ܡ�
void Draw_Element(class GUI_Element *tmp, HDC hdc, HWND hwnd) {

  if (tmp->Name == "RECTANGLE" || tmp->Name == "RECT" || tmp->Name == "ELIPSE" || tmp->Name == "LINE") {
	
	//����ֵ
	int pt = tmp->parent->t;
	int pl = tmp->parent->l;
	int pr = tmp->parent->r;
	int pb = tmp->parent->b;
	
	int l = atof(tmp->Property["left"].c_str()) * (pr - pl) + pl;
	int t = atof(tmp->Property["top"].c_str()) * (pb - pt) + pt;
	int r = atof(tmp->Property["right"].c_str()) * (pr - pl) + pl;
	int b = atof(tmp->Property["bottom"].c_str()) * (pb - pt) + pt;
	
	cout << tmp->parent->Name << " parent name .. \n";
	cout << pl << " "<< pt << " " << pr  << " " << pb <<"  parent ... RECTANGLE\n";
	cout << l << " "<< t  << " " << r  << " " << b <<" local ... RECTANGLE\n";
	
	tmp->l = l;
	tmp->t = t;
	tmp->r = r;
	tmp->b = b;
	
	//RECT��RECTANGEL��������ǰ��ֻ����λʹ�ã��������ƣ����߻���ƾ��Ρ�
	if (tmp->Name == "RECT") {   
	//����ֵ
	}
	
	if (tmp->Name == "RECTANGLE") {
/*	
		m_pBrush = new SolidBrush(Color(128, GetRValue(clrMask), GetGValue(clrMask), GetBValue(clrMask))); // ͸���� 128 ����2��ʹ�û�ˢ��ͼ
		Graphics graphics(hDC);
		graphics.FillPolygon(&m_pBrush, pts, 3, FillModeAlternate);
*/
		HBRUSH hbrush;
		HPEN hpen;

		hpen = CreatePen(PS_SOLID, 10, RED_COLOR);  // ������ɫʵ��10px�ֵĻ���
		hbrush = CreateSolidBrush(BLUE_COLOR); // ������ɫ��ˢ
		
		SelectObject(hdc, hpen);    // ָ������
		SelectObject(hdc, hbrush);  // ָ����ˢ
		
		Rectangle(hdc,l,t,r,b);
		
		// ������Դ
		DeleteObject(hpen);
		DeleteObject(hbrush);
	}
	
	//����ֱ��
	if (tmp->Name == "LINE") {
		MoveToEx(hdc, l, t, NULL);
		LineTo(hdc, r, b);
	}
	
	if (tmp->Name == "ELIPSE") {    
		Ellipse(hdc,l,t,r,b);
	}
  }
  
  if (tmp->Name == "BEZIER") {
	POINT apt[4];
	
	apt[0].x = 10;
    apt[0].y = 10;

    apt[1].x = 50;
    apt[1].y = 50;

    apt[2].x = 80;
    apt[2].y = 80;

    apt[3].x = 200;
    apt[3].y = 200;
	
	DrawBezier(hdc, apt);
  }
  
	//GDI �ı�����ͨ���ṩ�� GDI + ���õ����ܺ͸�׼ȷ���ı�������
  if (tmp->Name == "TEXT") {
	  
	Graphics *g = new Graphics(hdc);
	
    string s;
    
    s = tmp->Property["caption"];

    for (auto it: var) {
      if (it.first == s) {
        s = it.second;
      }
    }
   
    tmp->t = tmp->parent->t;
    tmp->l = tmp->parent->l;
    tmp->r = tmp->parent->r;
    tmp->b = tmp->parent->b;
    
    RECT re;  
    re.left=tmp->l; re.top=tmp->t; re.right=tmp->r; re.bottom=tmp->b;
    
    int i;
    if (s.substr(0, 4) == "_VAL")
       i = atoi(s.substr(4).c_str());
   

	SetTextColor(hdc,RGB(0,255,0));
	SetBkColor(hdc, 0x0000FF);
	//���ñ�����ɫΪ��ɫ
    //SetBkMode(hdc,TRANSPARENT);
	//���ñ���͸��
	  
	 // ������ʾ DT_SINGLELINE
	 //������ʾ, ����\r\n���Զ�������ʾ
	 //������ʾ�������߽��Զ�������ʾ
    //TextOut(hdc, l, t, s.c_str(), s.length());
    DrawText(hdc, Val[i].c_str(), Val[i].length(), &re, DT_LEFT|DT_END_ELLIPSIS | DT_EDITCONTROL | DT_WORDBREAK);
  }
  
  //����
  if (tmp->Name == "A") {
    cout<< "A...\n";
  }
  
  if (tmp->Property["sleep"]!="")
	Sleep(atoi(tmp->Property["sleep"].c_str()));
	
  if (tmp->child)
    Draw_Element(tmp->child, hdc, hwnd);
  if (tmp->brother)
    Draw_Element(tmp->brother, hdc, hwnd);
}

//�ҵ�hwnd��Ӧ��Window_Element�������±꣬�������-1��ʾû���ҵ���
int Find_Window(HWND hwnd) {
  for (int i=0; i< v_window.size(); i++){
    if (v_window[i]->hwnd == hwnd) {
      return i;
    }
  }
  return -1;
}

//��ȡGUI�����ļ����������νṹ��
void read_gui(char *gui){
  ifstream in(gui);
  string line;
      
  MyAssert(Get_First("hello world"), "hello");
  
  if (in) {
      
    class GUI_Element *last = NULL;
    // line�в�����ÿ�еĻ��з�
    while (getline (in, line)) {
    
      line = Skip_Blank(line);
      cout << line << endl;
          
      if (line == "")
        continue;
	
      // ����ͷ��ʾע��
      if (line[0] == ';')
        continue;                
        
      //��������
      if (line=="@VAR") {
        while (getline (in, line)) {
          line = Skip_Blank(line);
          cout << line << "\n";          
          
          if (line == "")
            continue;
          if (line[0] == ';')
            continue;
          if (line == "END")
            break;
          
          string l = String_2_Int(line);
          cout << l << " ..\n";
          
          string f = Get_First(l);
          //�������һ������
          while (f != l) {
            Evaluate(f);
            l = Skip_Blank(l.substr(f.length()));
            f = Get_First(l);
          }
          Evaluate(f);
        }
      }

      //ͼ������
      if (line=="@GUI") {
        while (getline (in, line)) {
          line = Skip_Blank(line);
          cout << line << "\n";
					
          if (line == "")
            continue;
          if (line[0] == ';')
            continue;   
          if (line=="END")
            break;          
          
          string l = String_2_Int(line);
          cout << l << "\n";

          class GUI_Element *con = new GUI_Element(l);
          
          //����ؼ�
          if (con->Level == 1 && con->Name=="WINDOW") {
            con->parent=NULL;
            con->child=NULL;
            con->brother=NULL;
            last = con;
            struct Window_Element *e = (struct Window_Element *)malloc(sizeof(struct Window_Element));
            e->hwnd = 0;
            e->head = con;
            v_window.push_back(e);
            continue;
          } 
          
          //�Ƕ���ؼ�
          if (last != NULL) {
            //last���ӿؼ�
            if (con->Level - last->Level == 1) {
              con->parent=last;
              con->child=NULL;
              con->brother=NULL;
              last->child = con;
              last = con;
              continue;
            }

            //last���ֵܿؼ�
            if (con->Level == last->Level) {
              last->brother=con;
              
              con->parent=last->parent;
              con->child=NULL;
              con->brother=NULL;
              last = con;
              continue;
            }
            //last�ϲ�ؼ�
            if (con->Level < last->Level && con->Level > 1) {
              while (con->Level != last->Level)
                last = last->parent;
              last->brother = con;
              con->parent=last->parent;
              con->child=NULL;
              con->brother=NULL;
              last = con;
              continue;
            }
            //�ؼ���δ���
            cout << con->Level  << "..\n";
            cout << last->Level  << "..\n";
            if (con->Level - last->Level > 1) {
              cout << "Level error.\n";
              return;
            }
          }
        }
      }

/*
      if (line=="@ZHUSHI") {
		while (getline (in, line)) {
		  line = Skip_Blank(line);
            cout << line << " ............................... \n";
          if (line == "")
            continue;
          if (line[0] == ';')
            continue;
          if (line=="END")
            break;
		}
	  }
	*/
	
      //��ʼ������
      if (line=="@INIT") {
        while (getline (in, line)) {
          line = Skip_Blank(line);
          cout << line << "\n";
          if (line == "")
            continue;
          if (line[0] == ';')
            continue;
          if (line=="END")
            break;
          string l = String_2_Int(line);
          continue;
        }
      }

      //��@��ͷ������Ϊ@init--��ʼ���������߼�����괦������
      if (line[0] == '@') {
        class Proc *p = new(class Proc);
        p->name = line.substr(1);
        
        while (getline (in, line)) {
          line = Skip_Blank(line);
          cout << line << "\n";
          if (line == "")
            continue;
          if (line[0] == ';')
            continue;
          if (line=="END")
            break;
          
          string l = String_2_Int(line);

          string f = Get_First(l);
          //�������һ������
          while (f != l) {
            p->s.push_back(f);
            l = Skip_Blank(l.substr(f.length()));
            f = Get_First(l);
          }
        }
        vp.push_back(p);
      }
    }
  } else {
      cout <<"no such file" << endl;
  }
}

int main(int argc, char **argv) {
  SetConsoleOutputCP(65001);
  cxScreen=GetSystemMetrics(SM_CXSCREEN);
  cyScreen=GetSystemMetrics(SM_CYSCREEN);
  if (argc == 1) {
    cout << "Need file name as args.\n";
    exit(1);
  }
  read_gui(argv[1]);
  
  WNDCLASS            wndClass;
  
  wndClass.style          = CS_HREDRAW | CS_VREDRAW;
  //ͬһ�����๫��һ�����ڴ������
  wndClass.lpfnWndProc    = WndProc;
  wndClass.cbClsExtra     = 0;
  wndClass.cbWndExtra     = 0;
  wndClass.hInstance      = NULL;
  wndClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
  wndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wndClass.lpszMenuName   = NULL;
  wndClass.lpszClassName  = TEXT("MyClass");
  
  RegisterClass(&wndClass);
  v_window[0]->Draw_Window();

  //�ͷ��ڴ�
  
  return 0;
}

//����p�����Ŀؼ�
GUI_Element * Find_Element(POINT p, GUI_Element *e) {
  GUI_Element *last = NULL;

  cout << "e->name = " << e->Name << "\n";
  
  if ((p.x >= e->l) && (p.y >= e->t) && (p.x <= e->r) && (p.y <= e->b))
      last = e;
    
  if (e->child != NULL) {
    if (e->child->Name == "RECTANGLE") {
      GUI_Element *tmp = Find_Element(p, e->child);
      if (tmp!=NULL)
        last = tmp;
    }
  } 
  
  if (e->brother!= NULL) {
    if (e->brother->Name == "RECTANGLE") {
      GUI_Element *tmp = Find_Element(p, e->brother);
      if (tmp!=NULL)
        last = tmp;
    }
  } 
  
  return last;
}

// vp�������Ϊname�Ĺ���
class Proc * Find_Proc(string name) {
		for (auto i:vp) {
			if (i->name == name)
				return i;
		}
		return NULL;
}

//ͬһ�������๫��һ�����ڴ������
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, 
   WPARAM wParam, LPARAM lParam)
{
  int i;
  string s;
  RECT r;  
  
  switch(message) {
    //�ƶ���������
    case WM_SYSCOMMAND:
      i = Find_Window(hWnd);
      
      //ȡ�ô��ڿͻ�������
      GetClientRect(hWnd, &r);
      
      //ȡ���ƶ����޸Ĵ���Ŀͻ������ꡣ
      v_window[i]->head->l = r.left;
      v_window[i]->head->t = r.top;
      v_window[i]->head->t = r.right;
      v_window[i]->head->b = r.bottom;
			
      //InvalidateRect(hWnd, NULL, TRUE);
      
      return DefWindowProc(hWnd, message, wParam, lParam);
      
    case WM_LBUTTONDOWN:
      
      POINT point;
      
      GetCursorPos(&point);            // ��ȡ���ָ��λ�ã���Ļ���꣩
      ScreenToClient(hWnd, &point);    // �����ָ��λ��ת��Ϊ��������
      cout << point.x << " "<< point.y << "\n";
      
      i = Find_Window(hWnd);
      //����Window_Element���õ�����������ڵĿؼ����������䴦������
      //������ʽ���ȸ����ӣ����ֺ�ܡ�
      class GUI_Element * tmp;
      tmp = Find_Element(point, v_window[i]->head);
      
      s = tmp->Property["click"];
			cout << s  << " s = \n";
      int j;
      if (s.substr(0, 4) == "_VAL") {
        j = atoi(s.substr(4).c_str());
        cout<< j << " j =\n";
      }
      cout << Val[j] << " is clicked\n";
	  class Proc * p;
	  p = Find_Proc(Val[j]);
      if (p!=NULL)
		p->Call();	
	  return 0;
    case WM_LBUTTONDBLCLK:
      return 0;
    case WM_RBUTTONDOWN:
      return 0;
    case WM_RBUTTONDBLCLK:
      return 0;
    case WM_MBUTTONDOWN:
      return 0;
    case WM_MBUTTONDBLCLK:
      return 0;
    //����Edit�ؼ����ı�
    case WM_SETTEXT:
      return 0;
    case WM_TIMER: 
      //InvalidateRect(hWnd, NULL, TRUE);
      return 0;
      
    // WINDOW����ͼ��Ԫ�صĻ���
    case WM_PAINT:
      PAINTSTRUCT pt;
      HDC hdc;
	  HPEN hpen; // ����
      
      hdc=BeginPaint(hWnd,&pt);

	  // ��������
      hpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
	  // ѡ�л���
      SelectObject(hdc, hpen);
      
      i = Find_Window(hWnd);
            
      //ȡ�ô��ڿͻ�������
      GetClientRect(hWnd, &r);
            
      //�洢��ǰ���ڿͻ�����top left right bottom
      v_window[i]->head->l = r.left;
      v_window[i]->head->t = r.top;
      v_window[i]->head->r = r.right;
      v_window[i]->head->b = r.bottom;

	  //Sleep(atoi(v_window[i]->head->Property["sleep"].c_str()));
      Draw_Element(v_window[i]->head->child, hdc, hWnd);
      
	  // ɾ������
      DeleteObject(hpen);
	  
      EndPaint(hWnd,&pt);
	  return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}