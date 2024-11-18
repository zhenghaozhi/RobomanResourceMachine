#include <iostream>
#include <windows.h>
#include <cwchar>
#include <conio.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <math.h>
#include <stdlib.h>
#include <Mmsystem.h>
#pragma comment (lib,"winmm.lib") 
using namespace std;

void setsize(int col, int row) {
	char cmd[64];
	sprintf_s(cmd, "mode con cols=%d lines=%d", col, row);
	system(cmd);
	SetWindowLongPtrA(GetConsoleWindow(), GWL_STYLE, GetWindowLongPtrA(GetConsoleWindow(), GWL_STYLE) & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX);
}
VOID HideConsoleCursor(VOID) {
	CONSOLE_CURSOR_INFO cursor_info = { 1, 0 };
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE),
		&cursor_info);
}
VOID ReviveConsoleCursor(VOID) {
	CONSOLE_CURSOR_INFO cursor_info = { 1, 1 };
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE),
		&cursor_info);
} //windows控制台API

string cmd_array[300]; //切分后的指令字符串
string cmd_para[300]; //切分后的参数字符串
string instruction;
int rob_x, rob_y, holding_tmp = 0;
bool if_holding;
//机器人相关变量，分别为：位置x坐标；位置y坐标；持有内容;是否持有盒子
int input_array[300], output_array[300], expected_array[300], memory[4] = { 0 };
//输入盒子序列；玩家实际输出序列；期望答案输出序列；空地序列
int input_sum, ans_sum, input_tmp = 0, output_tmp = 0, ready_RAM;
//输入、输出、空地相关变量：输入盒子总数；期望输出盒子数；当前已使用的输入盒子数；当前实际已输出的盒子数；本关可用空地数
int cmd_tmp = 1, last_cmd_tmp = 0, cmd_sum = 0, available_num, cmd_cnt, usable[9];
//指令相关变量：当前所在指令行数；上一条执行指令的行数；指令总数；可用指令种数；本关总计指令执行次数;可用指令序号序列（小到大）
int achievement_array[6][3];
//用于存储保存在关卡参数中的成就要求
bool if_usable[300], if_memory_use[300];
//可用情况变量：第i种指令是否可用；第i位空地是否占用
bool achievements[6][3], voices = 1, step_trace = 0;
//存储用户是否达成某一关的成就；是否开启音乐；是否开启单步跟踪
bool abortion = 0, winning = 0;
//结算判断：意外终止或失败；成功
int passed_level, chosen_level, input_mode = 2, set_ready, pages = 1;
//其他变量：已通关关卡；目前选中关卡;指令输入方式;关卡指令输入是否已完成（若未完成不进入结算界面，直接返回主界面）；指令页数
int paratemp[300], read_pin = 2, current_page = 1; // 临时存储参数
fstream save;
ifstream level_init;
ofstream save_in;
ifstream user_pref;
ofstream user_pref_in;
ifstream firstrunning;
ofstream firstrunning_in;
//文件读写变量：存档（读取）；关卡参数（读取）；存档（写入）存档文件名；用户偏好设置（读取）；用户偏好设置（写入）
int speed = 100, speed_sign = 2, speed_pause_tmp = 0;
//动画速度相关变量： speed_sign有0,1,2,4,8,16,32,默认为2,/2后对应当前速度倍率,=0时表示暂停; speed放入所有Sleep表示停顿间隔; speed_pause_tmp标示是否暂停, 默认为0
int speed_key_detect = int('0');
// 实时监测键盘输入
bool ifresetting = 0, ifhelping = 0, ifforus = 0, ifrestarting = 1; // 标记是否启动了存档清空, 是否进入了帮助二级菜单, 是否进入for_us, 是否重开
char level_gui[85][35], menu_gui[85][35]; // 用于缓存关卡和主菜单页面





void SetCoord(int x, int y) {
	HANDLE hOut;
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);//获取标注输出句柄
	COORD pos;
	pos.X = x; pos.Y = y;
	SetConsoleCursorPosition(hOut, pos);//偏移光标位置
} //以坐标形式设置光标位置
void rect_clear(int x1, int y1, int x2, int y2) {
	for (int i = y1; i <= y2; i++) {
		SetCoord(x1, i);
		//for (int j = x1; j <= x2; j++) cout << ' ';
		for (int j = x1; j <= x2; j++) printf(" ");
	}
}//清空矩形区域，(x1,y1)为左上顶点坐标，(x2,y2)为右下顶点坐标
void print_box(int x, int y, int content, bool if_num) {
	SetCoord(x, y);
	cout << "+---+";
	SetCoord(x, y + 1);
	if (if_num == 0) cout << "|   |";
	else {
		if (content >= 0 && content <= 9) cout << "| " << content << " |";
		else if (content < 0 && content >= -9) cout << "|" << content << " |";
		else cout << "|" << setw(3) << content << "|";
	}//规范两位以内负数、三位以内正整数的输出格式
	SetCoord(x, y + 2);
	cout << "+---+";
}//盒子输出函数，(x,y)为左上顶点坐标，content为盒子内数字，if_num判断是否要在盒中写入数字（若if_num==0，输出空盒子）
void out_boxstring_animat(int min_y) {
	rect_clear(43, 6, 49, 23);//清除右传送带
	if (output_tmp <= 5) {
		for (int i = 0; i < output_tmp; i++) {
			print_box(44, 3 * i + min_y, output_array[output_tmp - i], 1);
		}
	}//无盒子离开屏幕
	if (output_tmp >= 6) {
		for (int i = 0; i < 5; i++) {
			print_box(44, 3 * i + min_y, output_array[output_tmp - i], 1);
		}
		if (min_y == 6) {
			print_box(44, 21, output_array[output_tmp - 5], 1);
		}
		if (min_y == 7) {
			SetCoord(44, 22);
			cout << "+---+";
			SetCoord(44, 23);
			if (output_array[output_tmp - 5] >= 0 && output_array[output_tmp - 5] <= 9) cout << "| " << output_array[output_tmp - 5] << " |";
			else if (output_array[output_tmp - 5] < 0 && output_array[output_tmp - 5] >= -9) cout << "|" << output_array[output_tmp - 5] << " |";
			else cout << "|" << setw(3) << output_array[output_tmp - 4] << "|";
		}
		if (min_y == 8) {
			SetCoord(44, 23);
			cout << "+---+";
		}
	}//有盒子离开屏幕，依据坐标情况对最下方盒子做出特判
	Sleep(speed);
}//输出传送带基本动画，每执行一次右传送带所有盒子下降1格，最下方盒子超出界面边界时进行截断
void out_boxstring_winning_animat() {
	int nowbox = min(output_tmp, 6);
	int proceeded_box = output_tmp - nowbox + 1;
	for (int j = 6; j <= 24; j++) {
		rect_clear(43, 6, 49, 23);//清除右传送带
		if (j + nowbox * 3 - 1 <= 23) {
			for (int i = 0; i < nowbox; i++) {
				print_box(44, 3 * i + j, output_array[output_tmp - i], 1);
			}
		}
		else {
			for (int i = 0; i < nowbox - 1; i++) {
				print_box(44, 3 * i + j, output_array[output_tmp - i], 1);
			}
		}
		if (j + nowbox * 3 - 1 == 24) {
			SetCoord(44, 22);
			cout << "+---+";
			SetCoord(44, 23);
			if (output_array[proceeded_box] >= 0 && output_array[proceeded_box] <= 9) cout << "| " << output_array[proceeded_box] << " |";
			else if (output_array[proceeded_box] < 0 && output_array[proceeded_box] >= -9) cout << "|" << output_array[proceeded_box] << " |";
			else cout << "|" << setw(3) << output_array[proceeded_box] << "|";
		}
		if (j + nowbox * 3 - 1 == 25) {
			SetCoord(44, 23);
			cout << "+---+";
			proceeded_box++;
			nowbox--;
		}
		Sleep(100);
	}
	Sleep(speed);
}//输出传送带获胜清屏动画，每执行一次右传送带所有盒子下降1格，最下方盒子超出界面边界时进行截断
void in_boxstring_animat(int min_y) {
	rect_clear(0, 6, 5, 23);
	int remain = input_sum - input_tmp;
	if (remain <= 6) {
		for (int i = 0; i < remain - 1; i++) {
			print_box(1, 3 * i + min_y, input_array[input_tmp + 2 + i], 1);
		}
	}//无盒子进入界面
	if (remain >= 7) {
		for (int i = 0; i < 5; i++) {
			print_box(1, 3 * i + min_y, input_array[input_tmp + 2 + i], 1);
		}
		if (min_y == 6) {
			print_box(1, 21, input_array[input_tmp + 7], 1);
		}
		if (min_y == 7) {
			SetCoord(1, 22);
			cout << "+---+";
			SetCoord(1, 23);
			if (input_array[input_tmp + 7] >= 0 && input_array[input_tmp + 7] <= 9) cout << "| " << input_array[input_tmp + 7] << " |";
			else if (input_array[input_tmp + 7] < 0 && input_array[input_tmp + 7] >= -9) cout << "|" << input_array[input_tmp + 7] << " |";
			else cout << "|" << setw(3) << input_array[input_tmp + 7] << "|";
		}
		if (min_y == 8) {
			SetCoord(1, 23);
			cout << "+---+";
		}
	}//有盒子进入界面
	Sleep(speed);
}//输入传送带基本动画，每执行一次右传送带所有盒子上升1格，盒子进入界面边界时只显示已进入部分

//以上函数负责基本图形操作

void print_robot(int x, int y, int hold_status, int mov_stat) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	SetCoord(x, y);
	cout << "+---+";
	SetCoord(x - 1, y + 1);
	cout << "(|-_-|)";
	SetCoord(x - 2, y + 2);
	if (hold_status == 1) cout << " \\+---+/";//有盒子，手臂上举
	else cout << "  +---+";
	SetCoord(x - 1, y + 3);
	if (hold_status == 0) cout << "/[___]\\";//未举盒子，手臂下垂
	else {
		cout << " [___]";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		print_box(x, y - 3, holding_tmp, 1);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	}
	SetCoord(x, y + 4);
	if (mov_stat == 0) cout << " | | ";
	else if (mov_stat == -1) cout << " | \\";//左移腿部
	else cout << " / |";//右移腿部
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}//在给定位置输出一个机器人，（x,y)为左上'+'符号位置，hold_stastus记录是否举盒子，mov_stat记录是否正在移动
void robot_horizon_mov(int dir) {
	rect_clear(max(0, rob_x - 1), max(0, rob_y - 3), max(0, rob_x + 5), max(0, rob_y + 4));
	rob_x = rob_x + dir;
	print_robot(rob_x, rob_y, if_holding, dir);
	Sleep(speed);
}//机器人水平移动，每执行一次移动一格（dir=1向右，dir=-1向左）
void robot_vertical_mov(int dir) {
	rect_clear(max(0, rob_x - 1), max(0, rob_y - 3), max(0, rob_x + 5), max(0, rob_y + 4));
	rob_y = rob_y + dir;
	print_robot(rob_x, rob_y, if_holding, 0);
	Sleep(speed);
}//机器人竖直移动，每执行一次移动一格（dir=1向下，dir=-1向上）
void robot_animat(int dest_index) {
	int dest_x, dest_y;
	if (dest_index == 0) { dest_x = 10; dest_y = 5; }
	if (dest_index == 1) { dest_x = 16; dest_y = 8; }
	if (dest_index == 2) { dest_x = 20; dest_y = 8; }
	if (dest_index == 3) { dest_x = 24; dest_y = 8; }
	if (dest_index == 4) { dest_x = 28; dest_y = 8; }
	if (dest_index == 5) { dest_x = 36; dest_y = 5; }
	if (dest_index == 11) { dest_x = 34; dest_y = 4; }
	if (dest_index == 12) { dest_x = 39; dest_y = 10; }
	while (rob_x > dest_x && abortion == 0) robot_horizon_mov(-1);
	while (rob_x < dest_x && abortion == 0) robot_horizon_mov(1);
	while (rob_y > dest_y && abortion == 0) robot_vertical_mov(-1);
	while (rob_y < dest_y && abortion == 0) robot_vertical_mov(1);
}//机器人动画总控制函数，给定目标点后逐步移动到目标。
//此函数将机器人可到达的点（如：空地区域、inbox区域等）进行编号，以便于指令执行模块调用而不必再考虑坐标问题

//以上函数负责机器人图形相关

void print_message_box(int l_x, int l_y, int r_x, int r_y, bool if_filled) {
	if (if_filled) rect_clear(l_x, l_y, r_x, r_y);
	if (l_x >= 0 && l_y >= 0) {
		SetCoord(l_x, l_y);
		cout << "+";
		for (int i = l_x + 1; i < r_x; i++) cout << "-";
		cout << "+" << endl;
	}
	for (int j = l_y + 1; j <= r_y - 1; j++) {
		if (l_x >= 0 && j >= 0) {
			SetCoord(l_x, j);
			cout << "|";
			if (!if_filled) SetCoord(r_x, j);
			else for (int i = l_x + 1; i < r_x; i++) cout << " ";
			cout << "|" << endl;
		}
	}
	if (l_x >= 0 && r_y >= 0) {
		SetCoord(l_x, r_y);
		cout << "+";
		for (int i = l_x + 1; i < r_x; i++) cout << "-";
		cout << "+" << endl;
	}
	return;
}
void locked_level_animat(int level_pos) {
	int block_pos_y = 10, block_pos_x = 24 + (level_pos - 1) * 7;
	SetCoord(block_pos_x, block_pos_y - 1);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
	cout << "·所选关卡未解锁";
	for (int i = 1; i <= 5; i++) {
		rect_clear(block_pos_x - 2, block_pos_y, block_pos_x + 6, block_pos_y + 2);
		print_box(block_pos_x - 1, block_pos_y, level_pos, 1);
		Sleep(30);
		rect_clear(block_pos_x - 2, block_pos_y, block_pos_x + 6, block_pos_y + 2);
		print_box(block_pos_x + 1, block_pos_y, level_pos, 1);
		Sleep(30);
	}
	rect_clear(block_pos_x - 2, block_pos_y, block_pos_x + 6, block_pos_y + 2);
	print_box(block_pos_x, block_pos_y, level_pos, 1);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	Sleep(500);
	SetCoord(block_pos_x - 1, block_pos_y + 1);
	cout << ">";
	SetCoord(block_pos_x, block_pos_y - 1);
	cout << "                   ";
	return;
}
void set_level_gui() {
	for (int j = 0; j <= 28; j++) {
		for (int i = 0; i <= 79; i++) {
			level_gui[i][j] = ' ';
		}
	}
	for (int i = 0; i < 29; i++) level_gui[50][i] = '|';
	for (int i = 0; i < 7; i++) level_gui[i][4] = '_';
	level_gui[2][5] = 'I', level_gui[3][5] = 'N', level_gui[7][5] = '\\';
	for (int i = 0; i < 19; i++) level_gui[7][6 + i] = '|';
	for (int i = 0; i < 7; i++) level_gui[43 + i][4] = '_';
	level_gui[42][5] = '/', level_gui[45][5] = 'O', level_gui[46][5] = 'U', level_gui[47][5] = 'T';
	for (int i = 0; i < 19; i++) level_gui[42][6 + i] = '|';
	for (int i = 0; i < 50; i++) level_gui[i][24] = '-';
	for (int i = 0; i < 10; i++) level_gui[51 + i][0] = '=';
	for (int i = 0; i < 10; i++) level_gui[70 + i][0] = '=';
	for (int i = 0; i < 10; i++) level_gui[51 + i][9] = '=';
	for (int i = 0; i < 10; i++) level_gui[70 + i][9] = '=';
	level_gui[16][8] = '+', level_gui[17][8] = '-', level_gui[18][8] = '-', level_gui[19][8] = '-', level_gui[20][8] = '+';
	level_gui[15][9] = '(', level_gui[16][9] = '|', level_gui[17][9] = '-', level_gui[18][9] = '_', level_gui[19][9] = '-', level_gui[20][9] = '|', level_gui[21][9] = ')';
	level_gui[16][10] = '+', level_gui[17][10] = '-', level_gui[18][10] = '-', level_gui[19][10] = '-', level_gui[20][10] = '+';
	level_gui[15][11] = '/', level_gui[16][11] = '[', level_gui[17][11] = '_', level_gui[18][11] = '_', level_gui[19][11] = '_', level_gui[20][11] = ']', level_gui[21][11] = '\\';
	level_gui[17][12] = '|', level_gui[19][12] = '|';
	// 小机器人储存完毕
	level_gui[19][22] = '+';
	for (int i = 0; i < 9; i++) level_gui[20 + i][22] = '-';
	level_gui[29][22] = '+';
	level_gui[18][23] = '/', level_gui[30][23] = '\\';
	level_gui[21][23] = 'L', level_gui[22][23] = 'E', level_gui[23][23] = 'V', level_gui[24][23] = 'E', level_gui[25][23] = 'L';
	// 关卡名打印完毕
}
void set_menu_gui() {
	for (int j = 0; j <= 28; j++) {
		for (int i = 0; i <= 79; i++) {
			menu_gui[i][j] = ' ';
		}
	}
	menu_gui[28][5] = 'R', menu_gui[29][5] = 'o', menu_gui[30][5] = 'b', menu_gui[31][5] = 'o', menu_gui[32][5] = 'm', menu_gui[33][5] = 'a', menu_gui[34][5] = 'n';
	menu_gui[36][5] = 'R', menu_gui[37][5] = 'e', menu_gui[38][5] = 's', menu_gui[39][5] = 'o';
	menu_gui[40][5] = 'u', menu_gui[41][5] = 'r', menu_gui[42][5] = 'c', menu_gui[43][5] = 'e';
	menu_gui[45][5] = 'M', menu_gui[46][5] = 'a', menu_gui[47][5] = 'c', menu_gui[48][5] = 'h', menu_gui[49][5] = 'i', menu_gui[50][5] = 'n', menu_gui[51][5] = 'e';
	for (int i = 0; i < 5; i++) {
		menu_gui[24 + 7 * i][10] = '+';
		menu_gui[24 + 7 * i + 1][10] = '-';
		menu_gui[24 + 7 * i + 2][10] = '-';
		menu_gui[24 + 7 * i + 3][10] = '-';
		menu_gui[24 + 7 * i + 4][10] = '+';
	}
	for (int i = 0; i < 5; i++) {
		menu_gui[24 + 7 * i][11] = '|';
		menu_gui[24 + 7 * i + 2][11] = i + '1';
		menu_gui[24 + 7 * i + 4][11] = '|';
	}
	for (int i = 0; i < 5; i++) {
		menu_gui[24 + 7 * i][12] = '+';
		menu_gui[24 + 7 * i + 1][12] = '-';
		menu_gui[24 + 7 * i + 2][12] = '-';
		menu_gui[24 + 7 * i + 3][12] = '-';
		menu_gui[24 + 7 * i + 4][12] = '+';
	}
	//menu_gui[27][19] = '|', menu_gui[32][19] = '|';
	//menu_gui[37][19] = '|', menu_gui[42][19] = '|';
	//menu_gui[47][19] = '|', menu_gui[52][19] = '|';
}
void level_chosen_complete(int level_pos) {
	HideConsoleCursor();
	int block_pos_y = 10, block_pos_x = 24 + (level_pos - 1) * 7;
	rect_clear(block_pos_x, block_pos_y, block_pos_x + 4, block_pos_y + 2);
	SetCoord(block_pos_x + 1, block_pos_y + 1);
	cout << "[" << level_pos << "]";
	Sleep(100);
	int l_tmp, u_tmp, r_tmp, d_tmp;
	int speed_arrays[15] = { 2, 6, 12, 20, 34, 55, 85 };
	for (int k = 0; k < 7; k++) {
		int tmp = speed_arrays[k];
		l_tmp = (block_pos_x - (block_pos_x + 1) * tmp / 100);
		r_tmp = (block_pos_x + 4 + (81 - block_pos_x - 4) * tmp / 100);
		u_tmp = (block_pos_y - (block_pos_y + 1) * tmp / 100);
		d_tmp = (block_pos_y + 2 + (30 - block_pos_y - 2) * tmp / 100);
		Sleep(tmp);
		for (int j = u_tmp + 1; j <= d_tmp - 1; j++) {
			SetCoord(l_tmp + 1, j);
			for (int i = l_tmp + 1; i <= r_tmp - 1; i++) {
				if (i >= 0 && i <= 7 && j >= 4 && j <= 23) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
				else if (i >= 42 && i <= 49 && j >= 4 && j <= 23) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				else if (i >= 16 && i <= 22 && j >= 8 && j <= 14) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				cout << level_gui[i][j];
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
		}
		//rect_clear(l_tmp, u_tmp, r_tmp, d_tmp);
		print_message_box(l_tmp, u_tmp, r_tmp, d_tmp, 0);
	}
	return;
}
void level_end(int level_pos) {
	HideConsoleCursor();
	int block_pos_y = 10, block_pos_x = 24 + (level_pos - 1) * 7;
	int l_tmp, u_tmp, r_tmp, d_tmp;
	int speed_arrays[15] = { 15, 45, 66, 80, 88, 94, 98 };
	for (int k = 0; k < 7; k++) {
		int tmp = speed_arrays[k];
		l_tmp = (1 + (block_pos_x + 1) * tmp / 100);
		r_tmp = (81 - (81 - block_pos_x - 4) * tmp / 100);
		u_tmp = (1 + (block_pos_y + 1) * tmp / 100) - 1;
		d_tmp = (29 - (29 - block_pos_y - 2) * tmp / 100) - 1;
		Sleep(tmp*2/5);
		//Sleep(1000);
		if (k == 5) print_message_box(l_tmp, u_tmp, r_tmp, d_tmp, 1);
		else print_message_box(l_tmp, u_tmp, r_tmp, d_tmp, 0);
		for (int j = 0; j < u_tmp; j++) {
			SetCoord(0, j);
			for (int i = 0; i <= 78; i++) {
				if (j == 5) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
				cout << menu_gui[i][j];
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
		}
		for (int j = u_tmp; j <= d_tmp; j++) {
			SetCoord(0, j);
			for (int i = 0; i < l_tmp; i++) {
				cout << menu_gui[i][j];
			}
		}
		for (int j = u_tmp; j <= d_tmp; j++) {
			SetCoord(r_tmp + 1, j);
			for (int i = r_tmp + 1; i <= 78; i++) {
				cout << menu_gui[i][j];
			}
		}
		for (int j = d_tmp + 1; j <= 28; j++) {
			SetCoord(0, j);
			for (int i = 0; i <= 78; i++) {
				cout << menu_gui[i][j];
			}
		}
	}
	rect_clear(block_pos_x - 2, block_pos_y - 2, block_pos_x + 6, block_pos_y + 4);
	print_message_box(block_pos_x, block_pos_y, block_pos_x + 4, block_pos_y + 2, 1);
	SetCoord(block_pos_x + 2, block_pos_y + 1);
	cout << chosen_level;
	Sleep(100);
	return;
}
void page_refresh(int paging) {
	rect_clear(51, 11, 79, 26);
	int minlimit = 15 * (paging - 1) + 1;
	int maxlimit = min(15 * paging, cmd_sum);
	for (int i = 1; i <= maxlimit - minlimit + 1; i++) {
		SetCoord(52, i + 10);
		cout << 15 * (paging - 1) + i << " " << cmd_array[15 * (paging - 1) + i] << " " << cmd_para[15 * (paging - 1) + i];
	}
	SetCoord(72, 25);
	cout << "[" << paging << "/" << pages << "]";
}
void page_refresh_special(int paging) {
	rect_clear(51, 11, 79, 26);
	int minlimit = 15 * (paging - 1) + 1;
	int maxlimit = min(15 * paging, cmd_sum);
	for (int i = 1; i <= maxlimit - minlimit + 1; i++) {
		SetCoord(52, i + 10);
		cout << 15 * (paging - 1) + i << " " << cmd_array[15 * (paging - 1) + i];
		if (paratemp[15 * (paging - 1) + i] != -1) {
			SetCoord(67, 10 + i);
			cout << paratemp[15 * (paging - 1) + i];
		}
	}
	SetCoord(72, 25);
	cout << "[" << paging << "/" << pages << "]";
}
void game_initiate_animat() {
	HideConsoleCursor();
	for (int i = 0; i <= 19; i++) {
		for (int j = 0; j < 5; j++) { // time : i + j*2
			if (i < j * 2) continue;
			if (i == j * 2) {
				SetCoord(24 + 7 * j, 0);
				cout << "+---+";
			}
			else if (i == j * 2 + 1) {
				SetCoord(24 + 7 * j, 0);
				cout << "|   |";
				SetCoord(24 + 7 * j, 1);
				cout << "+---+";
			}
			else if (i == j * 2 + 12) {
				rect_clear(24 + 7 * j, 0, 24 + 7 * j + 4, i - j * 2 - 3);
				print_box(24 + 7 * j, i - j * 2 - 2, j + 1, 1);
			}
			else if (i > j * 2 + 12) continue;
			else {
				rect_clear(24 + 7 * j, 0, 24 + 7 * j + 4, i - j * 2 - 3);
				print_box(24 + 7 * j, i - j * 2 - 2, 0, 0);
			}
		}
		Sleep(50);
	}
	rect_clear(52, 0, 56, 9);
}
//以上函数负责动画相关

void inbox_init_v1() {
	if (input_sum <= 6) {
		for (int i = 1; i <= input_sum; i++) {
			print_box(1, 3 * i + 3, input_array[i], 1);
		}
	}
	else {
		for (int i = 1; i <= 6; i++) {
			print_box(1, 3 * i + 3, input_array[i], 1);
		}
	}
}//输入传送带初始化
void inbox_renew_v1() {
	rect_clear(0, 6, 5, 8);
	if_holding = 1;
	holding_tmp = input_array[input_tmp + 1];
	rect_clear(rob_x - 1, rob_y, rob_x + 5, rob_y + 4);
	print_robot(rob_x, rob_y, 1, 0);
	for (int i = 9; i >= 6; i--) {
		if (abortion == 1) break;
		in_boxstring_animat(i);
	}
}//输入传送带内容更新
void outbox_renew_v1() {
	for (int i = 6; i <= 9; i++) {
		if (abortion == 1) break;
		out_boxstring_animat(i);
	}
	print_box(44, 6, holding_tmp, 1);
	if_holding = 0;
	rect_clear(rob_x - 1, rob_y - 3, rob_x + 5, rob_y + 4);
	print_robot(rob_x, rob_y, 0, 0);
}//输出传送带内容更新
void RAM_renew(int RAM_index) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
	print_box(16 + 4 * RAM_index, 13, memory[RAM_index], 1);
	rect_clear(rob_x - 1, rob_y - 3, rob_x + 5, rob_y + 4);
	print_robot(rob_x, rob_y, if_holding, 0);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	Sleep(speed);
}

//以上函数负责输入输出及空地维护

void gaming_init() {
	for (int i = 0; i < 29; i++) {
		SetCoord(50, i);
		cout << '|';
	}//分界栏

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
	SetCoord(0, 4);
	cout << "_______";
	SetCoord(2, 5);
	cout << "IN   \\";
	for (int i = 0; i < 19; i++) {
		SetCoord(7, 6 + i);
		cout << "|";
	}//左传送带
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	SetCoord(43, 4);
	cout << "_______";
	SetCoord(42, 5);
	cout << "/  OUT";
	for (int i = 0; i < 19; i++) {
		SetCoord(42, 6 + i);
		cout << "|";
	}//右传送带
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	SetCoord(0, 24);
	for (int i = 0; i < 50; i++) {
		cout << "-";
	}//分界栏
	inbox_init_v1();

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
	for (int i = 0; i < ready_RAM; i++) {
		print_box(16 + 4 * i, 13, NULL, 0);
		SetCoord(18 + 4 * i, 16);
		cout << i;
	}//空地
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	print_robot(16, 8, 0, 0);
	rob_x = 16; rob_y = 8;
	SetCoord(51, 0);
	available_num = 1;
	cout << "==========可用指令==========";
	for (int i = 1; i <= 8; i++) {
		if (if_usable[i] == 0) continue;
		SetCoord(52, available_num);
		cout << available_num << ' ';
		if (i == 1) cout << "inbox";
		if (i == 2) cout << "outbox";
		if (i == 3) cout << "add";
		if (i == 4) cout << "sub";
		if (i == 5) cout << "copyto";
		if (i == 6) cout << "copyfrom";
		if (i == 7) cout << "jump";
		if (i == 8) cout << "jumpifzero";
		usable[available_num] = i;
		available_num++;
	}//可用指令输出

	SetCoord(19, 22);
	cout << "+";
	for (int i = 1; i <= 9; i++) cout << "-";
	cout << "+";
	SetCoord(18, 23);
	cout << "/";
	SetCoord(21, 23);
	cout << "LEVEL " << chosen_level;
	SetCoord(30, 23);
	cout << "\\";
	// 关卡信息输出
}//关卡界面初始化
void menu_init() {
	HideConsoleCursor();
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
	SetCoord(28, 5);
	cout << "Roboman Resource Machine";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	for (int i = 0; i < 5; i++) {
		if (i <= passed_level) print_box(24 + 7 * i, 10, i + 1, 1);
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
			print_box(24 + 7 * i, 10, i + 1, 1);
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			//SetCoord(26 + 7 * i, 13);
			//cout << 'X';
		}
		if (achievements[i + 1][1]) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
			SetCoord(26 + 7 * i - 1, 13);
			cout << "*";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
		if (achievements[i + 1][2]) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x06);
			SetCoord(26 + 7 * i + 1, 13);
			cout << "*";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
	}
	SetCoord(27, 19);
	cout << "|设置|";
	SetCoord(37, 19);
	cout << "|帮助|";
	SetCoord(47, 19);
	cout << "|退出|";
}//主菜单初始化
void menu_pin(int n, bool mode) {
	if (n <= 5) {
		SetCoord(23 + 7 * (n - 1), 11);
		if (mode == 1) cout << '>';
		else cout << ' ';
	}
	else {
		if (n == 11) SetCoord(26, 19);
		if (n == 12) SetCoord(36, 19);
		if (n == 13) SetCoord(46, 19);
		if (mode == 1) cout << '>';
		else cout << ' ';
	}
}//主菜单光标更新函数
void setting_UI() {
	//keybd_event(VK_CONTROL, 0, 0, 0);
	Sleep(80);
	SetCoord(26, 19);
	cout << "|>";
	SetCoord(32, 19);
	cout << " ";
	Sleep(80);
	SetCoord(25, 19);
	cout << "| ";
	Sleep(80);
	SetCoord(25, 20);
	cout << "|";
	Sleep(80);
	SetCoord(25, 21);
	cout << "| ·输入方式： ";
	if (input_mode == 0) cout << "键盘输入";
	if (input_mode == 1) cout << "文件输入";
	if (input_mode == 2) cout << "光标输入";
	Sleep(80);
	SetCoord(25, 22);
	cout << "| ·音量与音效：";
	if (voices == 1) cout << " 开";
	else cout << " 关";
	Sleep(80);
	SetCoord(25, 23);
	cout << "| ·单步跟踪：";
	if (step_trace == 1) cout << " 开";
	else cout << " 关";
	Sleep(80);
	SetCoord(25, 24);
	cout << "| ·重置游戏";
	//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
	int pin = 0; // 0, 2, 3, 4, 5 对应不同的按键
	while (1) {
		if (_kbhit()) {//如果有按键按下，则_kbhit()函数返回真
			int temp;
			temp = _getch();//使用_getch()函数获取按下的键值
			if (temp == 27) {
				//keybd_event(VK_CONTROL, 0, 0, 0);
				Sleep(80);
				rect_clear(25, 24, 80, 29);
				Sleep(80);
				rect_clear(25, 23, 80, 29);
				Sleep(80);
				rect_clear(25, 22, 80, 29);
				Sleep(80);
				rect_clear(25, 21, 80, 29);
				Sleep(80);
				rect_clear(25, 20, 80, 29);
				Sleep(80);
				SetCoord(25, 19);
				cout << " |>";
				Sleep(80);
				SetCoord(26, 19);
				cout << ">|设置|";
				//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
				break;
			}
			if (temp == 224) {
				SetCoord(27, 19 + pin);
				if (pin == 0) cout << ' ';
				else cout << "·";
				int temp2 = _getch();
				if (temp2 == 80 && pin < 5) {
					if (pin == 0) pin++;
					pin++;
				}
				else if (temp2 == 72 && pin > 0) {
					if (pin == 2) pin--;
					pin--;
				}
				SetCoord(27, 19 + pin);
				cout << '>';
			}
			if (temp == 13) {
				if (pin == 0) {
					//keybd_event(VK_CONTROL, 0, 0, 0);
					Sleep(80);
					rect_clear(25, 24, 80, 29);
					Sleep(80);
					rect_clear(25, 23, 80, 29);
					Sleep(80);
					rect_clear(25, 22, 80, 29);
					Sleep(80);
					rect_clear(25, 21, 80, 29);
					Sleep(80);
					rect_clear(25, 20, 80, 29);
					Sleep(80);
					SetCoord(25, 19);
					cout << " |>";
					Sleep(80);
					SetCoord(26, 19);
					cout << ">|设置|";
					//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
					break;
				}
				if (pin == 2) {
					SetCoord(38, 21);
					cout << "<";
					SetCoord(47, 21);
					cout << ">";
					while (1) {
						if (_kbhit()) {
							int temp = _getch();
							if (temp == 224) {
								int temp2 = _getch();
								if (temp2 == 77 && input_mode < 2) input_mode++;
								else if (temp2 == 75 && input_mode > 0) input_mode--;
								else if (temp2 == 77 && input_mode == 2) input_mode = 0;
								else if (temp2 == 75 && input_mode == 0) input_mode = 2;
								//if (temp2 == 80 && chosen_level != 11) chosen_level = 11;
								//if (temp2 == 72 && chosen_level == 11) chosen_level = 3;
								SetCoord(39, 21);
								if (input_mode == 0) cout << "键盘输入";
								if (input_mode == 1) cout << "文件输入";
								if (input_mode == 2) cout << "光标输入";
								continue;
							}
							if (temp == 13 || temp == 27) break;
						}
					}
					SetCoord(38, 21);
					cout << " ";
					SetCoord(47, 21);
					cout << " ";
				}
				if (pin == 3) {
					SetCoord(40, 22);
					cout << "<";
					SetCoord(43, 22);
					cout << ">";
					while (1) {
						if (_kbhit()) {
							int temp = _getch();
							if (temp == 224) {
								int temp2 = _getch();
								if (temp2 == 77 || temp2 == 75) {
									if (voices) voices = 0;
									else voices = 1;
								}
								SetCoord(41, 22);
								if (voices) cout << "开";
								else cout << "关";
								continue;
							}
							if (temp == 13 || temp == 27) {
								if (!voices) PlaySound(NULL, NULL, SND_PURGE); //背景音乐重置
								if (voices) PlaySound(TEXT("menu.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP); //背景音乐启动
								break;
							}
						}
					}
					SetCoord(40, 22);
					cout << " ";
					SetCoord(43, 22);
					cout << " ";
				}
				if (pin == 4) {
					SetCoord(38, 23);
					cout << "<";
					SetCoord(41, 23);
					cout << ">";
					while (1) {
						if (_kbhit()) {
							int temp = _getch();
							if (temp == 224) {
								int temp2 = _getch();
								if (temp2 == 77 || temp2 == 75) {
									if (step_trace) step_trace = 0;
									else step_trace = 1;
								}
								SetCoord(39, 23);
								if (step_trace) cout << "开";
								else cout << "关";
								continue;
							}
							if (temp == 13 || temp == 27) break;
						}
					}
					SetCoord(38, 23);
					cout << " ";
					SetCoord(41, 23);
					cout << " ";
				}
				if (pin == 5) {
					SetCoord(38, 24);
					cout << "按Y确认重置，按N取消";
					while (1) {
						if (_kbhit()) {
							int temp_b;
							temp_b = _getch();
							if (temp_b == 'Y' || temp_b == 'y') {
								SetCoord(38, 24);
								cout << "                           ";
								SetCoord(38, 24);
								cout << "重置中.";
								Sleep(300);
								save_in.open("save.save");
								save_in << 0 << endl;
								for (int i = 1; i <= 5; i++) save_in << "0 0" << endl;
								save_in.close();
								cout << ".";
								Sleep(200);
								ifresetting = true;
								user_pref_in.open("preferences");
								user_pref_in << "2 1 0";
								user_pref_in.close();
								cout << ".";
								Sleep(500);
								system("cls");
								Sleep(1000);
								return;
							}
							if (temp_b == 'N' || temp_b == 'n' || temp_b == 27) break;
						}
					}
					SetCoord(27, 24);
					cout << ">重置游戏                                ";
				}
			}
		}
	}
}//设置界面函数
void ending_UI(int code) {
	Sleep(200);
	if (winning == 1) out_boxstring_winning_animat();
	if (winning == 1) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	else if (code == 6) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
	else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
	print_message_box(25, 10, 54, 20, 1);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	SetCoord(29, 12);
	if (winning == 1) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		cout << "SUCCESS";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		SetCoord(29, 14);
		cout << "指令行数：" << cmd_sum;
		if (cmd_sum <= achievement_array[chosen_level][1] && !achievements[chosen_level][1]) {
			SetCoord(43, 12);
			cout << "   得到：";
			SetCoord(45, 14);
			cout << "银勋章";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
			cout << "*";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			achievements[chosen_level][1] = true;
		}
		SetCoord(29, 16);
		cout << "总执行次数：" << cmd_cnt;
		if (cmd_cnt <= achievement_array[chosen_level][2] && !achievements[chosen_level][2]) {
			SetCoord(43, 12);
			cout << "   得到：";
			SetCoord(45, 16);
			cout << "金勋章";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x06);
			cout << "*";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			achievements[chosen_level][2] = true;
		}
		save_in.open("save.save");
		if (chosen_level > passed_level) save_in << chosen_level << endl;
		else save_in << passed_level << endl;
		for (int i = 1; i <= 5; i++) save_in << achievements[i][1] << " " << achievements[i][2] << endl;
		save_in.close();
		SetCoord(29, 18);
		system("pause");
		ifrestarting = 0;
		level_end(chosen_level);
		return;
	}
	else {
		if (code == -2) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			cout << "ERROR";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			SetCoord(29, 16);
			cout << "错误类型：指令数量超限";
			SetCoord(29, 18);
			system("pause");
			return;
		}
		else if (code == 5) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			cout << "WRONG ANSWER";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			SetCoord(29, 14);
			cout << "指令行数：" << cmd_sum;
			SetCoord(29, 16);
			cout << "指令执行总次数：" << cmd_cnt;
		}
		else if (code == 6) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
			cout << "ABORTION";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			SetCoord(29, 14);
			cout << "放弃，是一种境界";
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			cout << "ERROR";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			SetCoord(29, 14);
			if (code < 7) cout << "报错行数：" << cmd_tmp;
			SetCoord(29, 16);
			cout << "错误类型：";
			if (code == -1) cout << "语法错误";
			if (code == 0) cout << "指令不可用";
			if (code == 1) cout << "空手不可操作";
			if (code == 2) cout << "内存不可用";
			if (code == 3) cout << "内存为空";
			if (code == 4) cout << "目标指令非法";
			if (code == 7) cout << "指令数量非法";
			if (code == 8) cout << "给定指令数不符";
			if (code == 9) cout << "文件为空";
			if (code == 10) cout << "文件不存在";
			if (code == 11) cout << "数据超限";
			if (code == 12) cout << "执行过多";
			if (code == 13) cout << "文件名长度非法";
		}
	}
	SetCoord(29, 18);
	cout << ">重新开始";
	SetCoord(46, 18);
	cout << "·退出";
	HideConsoleCursor();
	int chosen_end_type = 0;
	while (1) {
		if (_kbhit()) {
			int temp = _getch();
			if (temp == 224) {
				int temp2 = _getch();
				if (temp2 == 77 && chosen_end_type < 1) chosen_end_type++;
				else if (temp2 == 75 && chosen_end_type > 0) chosen_end_type--;
				if (chosen_end_type == 0) {
					SetCoord(29, 18);
					cout << ">";
					SetCoord(46, 18);
					cout << "·";
				}
				if (chosen_end_type == 1) {
					SetCoord(29, 18);
					cout << "·";
					SetCoord(46, 18);
					cout << ">";
				}
				continue;
			}
			if (temp == 13) break;
		}
	}
	if (code == 13) system("cls");
	if (chosen_end_type == 1) {
		ifrestarting = 0;
		level_end(chosen_level);
	}
}//关卡结算界面
void for_us() {
	ifforus = 1;
	PlaySound(NULL, NULL, SND_PURGE); //背景音乐重置
	if (voices) {
		PlaySound(TEXT("credit.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	}//背景音乐启动
	ifstream about;
	string temp;
	about.open("Crdit.txt");
	SetCoord(0, 0);
	for (int i = 1; i <= 29; i++) {
		getline(about, temp);
		cout << temp << endl;
	}
	print_box(24, 2, 2, 1);
	SetCoord(38, 0);
	cout << "  | |  ";
	Sleep(200);
	SetCoord(38, 0);
	cout << "/[___]\\";
	SetCoord(38, 1);
	cout << "  | |  ";
	Sleep(200);
	SetCoord(38, 0);
	cout << " +---+ ";
	SetCoord(38, 1);
	cout << "/[___]\\";
	SetCoord(38, 2);
	cout << "  | |  ";
	Sleep(200);
	SetCoord(38, 0);
	cout << "(|-_-|)";
	SetCoord(38, 1);
	cout << " +---+ ";
	SetCoord(38, 2);
	cout << "/[___]\\";
	SetCoord(38, 3);
	cout << "  | |  ";
	Sleep(200);
	print_robot(39, 0, 0, 0);
	Sleep(200);
	rob_x = 39; rob_y = 0; if_holding = 0; speed = 200;
	robot_animat(11);
	rect_clear(24, 2, 28, 4);
	if_holding = 1; holding_tmp = 2;
	print_robot(rob_x, rob_y, 1, 0);
	robot_animat(12);
	about.close();

	for (int i = 1; i <= 114; i++) {
		about.open("Crdit.txt");
		SetCoord(0, 0);
		for (int j = 1; j < i; j++) {
			getline(about, temp);
		}
		for (int j = i; j <= i + 29; j++) {
			getline(about, temp);

			cout << temp << endl;
		}
		print_robot(rob_x, rob_y, 1, 0);
		Sleep(200);
		about.close();
	}

	for (int i = 1; i <= 8; i++) {
		robot_horizon_mov(1);
	}
	Sleep(50);
	rect_clear(rob_x, rob_y - 3, rob_x + 5, rob_y);
	print_robot(rob_x, rob_y, 0, 0);
	print_box(55, 13, 2, 1);
	Sleep(200);
	rect_clear(55, 13, 59, 15);
	print_box(55, 14, 2, 1);
	Sleep(200);
	rect_clear(55, 14, 59, 16);
	SetCoord(55, 15);
	cout << "+---+";
	SetCoord(55, 16);
	cout << "| 2 |";
	Sleep(200);
	SetCoord(55, 15);
	cout << "     ";
	SetCoord(55, 16);
	cout << "+---+";
	Sleep(200);
	SetCoord(55, 16);
	cout << "     ";
	Sleep(2000);
	SetCoord(34, 25);
	system("pause");
	system("cls");
	PlaySound(NULL, NULL, SND_PURGE); //背景音乐重置
	if_holding = 0; speed = 100, holding_tmp = 0;
}//关于界面
void guidance_assist(int page) {
	if (page == 1) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(10, 4); cout << "欢迎来到教程";
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 2) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 2); cout << "这里是主菜单，你可以使用      移动光标并以Enter键确认选择";
		SetCoord(16, 11); cout << "光标 → >";
		SetCoord(25, 13); cout << "*";
		SetCoord(32, 13); cout << "*";
		SetCoord(26, 14); cout << "↑";
		SetCoord(16, 15); cout << "成就系统：达成相应挑战可获得金/银勋章";
		SetCoord(64, 3); cout << "\\";
		SetCoord(12, 19); cout << "可选择选项卡 →";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(30, 2); cout << "方向键";
		SetCoord(48, 2); cout << "Enter";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(27, 13); cout << "*";
		SetCoord(34, 13); cout << "*";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
		print_box(52, 10, 5, 1);
		SetCoord(58, 11); cout << "← 未解锁关卡";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 3) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(10, 19); cout << "选择某选项卡 →";
		SetCoord(23, 21); cout << "↑";
		SetCoord(12, 22); cout << "键可移动光标";
		SetCoord(6, 23); cout << "键选中进入二级菜单";
		SetCoord(23, 24); cout << "↓";
		SetCoord(24, 2); cout << " 按下   或在选项卡上     均可收起选项卡";
		SetCoord(64, 3); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(29, 2); cout << "Esc";
		SetCoord(44, 2); cout << "Enter";
		SetCoord(8, 22); cout << "上下";
		SetCoord(1, 23); cout << "Enter";
		SetCoord(5, 19); cout << "Enter";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 4) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(23, 21); cout << "→";
		SetCoord(8, 21); cout << "选中此二级菜单";
		SetCoord(50, 21); cout << "左右键以切换不同选项";
		SetCoord(17, 2); cout << "按下Esc或Enter均可在二级菜单中保存并返回上一级";
		SetCoord(64, 3); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(21, 2); cout << "Esc";
		SetCoord(26, 2); cout << "Enter";
		SetCoord(50, 21); cout << "左右";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 5) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(48, 17); cout << "在主界面Esc等效于选中|退出|";
		SetCoord(49, 18); cout << "↓";
		SetCoord(49, 23); cout << "↑";
		SetCoord(20, 2); cout << "你可以在任何界面Esc，这通常意味着返回上一级";
		SetCoord(34, 24); cout << "特殊界面，请按提示操作";
		SetCoord(64, 3); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(36, 2); cout << "Esc";
		SetCoord(56, 17); cout << "Esc";
		SetCoord(69, 17); cout << "|退出|";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 6) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(25, 8); cout << "在已解锁关卡上Enter以进入关卡";
		SetCoord(26, 9); cout << "↓";
		SetCoord(10, 2); cout << "下面让我们进入关卡，学习如何游玩Roboman Resource Machine";
		SetCoord(64, 3); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(39, 8); cout << "Enter";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 7) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(47, 25); cout << "<-----+";
		for (int i = 15; i < 25; i++) { SetCoord(53, i); cout << "|"; }
		SetCoord(43, 28); cout << "<-----------------+";
		for (int i = 19; i < 28; i++) { SetCoord(61, i); cout << "|"; }
		SetCoord(6, 1); cout << "欢迎来到关卡，首先你应该注意到关卡信息";
		SetCoord(6, 2); cout << "它将告诉你如何通过关卡";
		SetCoord(11, 6); cout << "我会在这里接收你的指令并执行";
		SetCoord(53, 12); cout << "请按指示用指令操作机器人";
		SetCoord(53, 13); cout << "按要求完成搬运任务";
		SetCoord(57, 15); cout << "你需要精简你的代码";
		SetCoord(57, 16); cout << "达成相应挑战要求后";
		SetCoord(57, 17); cout << "将自动获得银/金勋章";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(70, 17); cout << "金勋章";
		SetCoord(36, 1); cout << "关卡信息";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 8) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 1); cout << "指令格式：inbox";
		SetCoord(6, 2); cout << "[无参数]";
		SetCoord(11, 5); cout << "这里是输入序列";
		SetCoord(11, 6); cout << "我可以通过inbox指令拿取积木";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(21, 6); cout << "inbox";
		SetCoord(16, 1); cout << "inbox";
		SetCoord(17, 5); cout << "输入序列";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
		SetCoord(0, 4);
		cout << "_______";
		SetCoord(2, 5);
		cout << "IN   \\";
		for (int i = 0; i < 18; i++) {
			SetCoord(7, 6 + i);
			cout << "|";
		}//左传送带
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 9) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 1); cout << "指令格式：outbox";
		SetCoord(6, 2); cout << "[无参数]";
		SetCoord(11, 5); cout << "这里是输出序列";
		SetCoord(11, 6); cout << "我可以通过outbox指令输出积木";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(21, 6); cout << "outbox";
		SetCoord(16, 1); cout << "outbox";
		SetCoord(17, 5); cout << "输出序列";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(43, 4);
		cout << "_______";
		SetCoord(42, 5);
		cout << "/  OUT";
		for (int i = 0; i < 18; i++) {
			SetCoord(42, 6 + i);
			cout << "|";
		}//右传送带
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 10) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 1); cout << "指令格式：copyto [X] / copyfrom [X]";
		SetCoord(6, 2); cout << "[X：内存编号]";
		SetCoord(11, 4); cout << "这里是内存矩阵";
		SetCoord(11, 5); cout << "我可以通过copyto指令暂存积木";
		SetCoord(11, 6); cout << "我可以通过copyfrom指令拿取积木";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(21, 5); cout << "copyto";
		SetCoord(21, 6); cout << "copyfrom";
		SetCoord(16, 1); cout << "copyto [X] / copyfrom [X]";
		SetCoord(17, 4); cout << "内存矩阵";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED);
		for (int i = 0; i < 3; i++) {
			print_box(16 + 4 * i, 13, NULL, 0);
			SetCoord(18 + 4 * i, 16);
			cout << i;
		}//空地
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 11) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 1); cout << "指令格式：add [X] / sub [X]";
		SetCoord(6, 2); cout << "[X：内存编号]";
		SetCoord(11, 4); cout << "在我手中与内存都不为空时";
		SetCoord(11, 5); cout << "我可以通过add/sub指令";
		SetCoord(11, 6); cout << "让手中方块加/减内存中储存的值";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(21, 5); cout << "add/sub";
		SetCoord(16, 1); cout << "add [X] / sub [X]";
		SetCoord(15, 4); cout << "手中与内存都不为空时";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED);
		for (int i = 0; i < 3; i++) {
			print_box(16 + 4 * i, 13, NULL, 0);
			SetCoord(18 + 4 * i, 16);
			cout << i;
		}//空地
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 12) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 1); cout << "指令格式：jump [Y] / jumpifzero [Y]";
		SetCoord(6, 2); cout << "[Y：目标指令行数]";
		SetCoord(11, 4); cout << "通过jump/jumpifzero指令";
		SetCoord(11, 5); cout << "我可以跳转到想要执行的下一指令";
		SetCoord(11, 6); cout << "运行至此时将会跳转后再继续运行";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(17, 5); cout << "跳转";
		SetCoord(33, 5); cout << "下一指令";
		SetCoord(16, 1); cout << "jump [Y] / jumpifzero [Y]";
		SetCoord(15, 4); cout << "jump/jumpifzero";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED);
		for (int i = 0; i < 3; i++) {
			print_box(16 + 4 * i, 13, NULL, 0);
			SetCoord(18 + 4 * i, 16);
			cout << i;
		}//空地
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 13) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(6, 1); cout << "指令格式：jump [Y] / jumpifzero [Y]";
		SetCoord(6, 2); cout << "[Y：目标指令行数]";
		SetCoord(11, 4); cout << "特别的，jumpifzero需要满足";
		SetCoord(11, 5); cout << "“我手中正拿着方块”且“方块为0”";
		SetCoord(11, 6); cout << "才会跳转，否则不跳转继续执行";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(11, 6); cout << "才会跳转";
		SetCoord(31, 5); cout << "“方块为0”";
		SetCoord(11, 5); cout << "“我手中正拿着方块”";
		SetCoord(16, 1); cout << "jump [Y] / jumpifzero [Y]";
		SetCoord(19, 4); cout << "jumpifzero";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED);
		for (int i = 0; i < 3; i++) {
			print_box(16 + 4 * i, 13, NULL, 0);
			SetCoord(18 + 4 * i, 16);
			cout << i;
		}//空地
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 14) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(17, 1); cout << "这里显示的是当前关卡可用的指令 →";
		SetCoord(11, 5); cout << "请注意，";
		SetCoord(11, 6); cout << "并非所有指令在每关中都可用";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(37, 1); cout << "可用的指令";
		SetCoord(11, 6); cout << "并非所有指令";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 15) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 19); cout << "设置 -> 输入方式";
		SetCoord(11, 4); cout << "根据你所选择的输入方式不同";
		SetCoord(11, 5); cout << "待执行指令区域的构成也会改变";
		SetCoord(11, 6); cout << "接下来让我们来依次介绍";
		SetCoord(52, 10); cout << "+-------------------------+";
		for (int i = 11; i < 28; i++) { SetCoord(52, i); cout << "|                         |"; }
		SetCoord(52, 28); cout << "+-------------------------+";
		SetCoord(14, 7); cout << "\\";
		SetCoord(59, 19); cout << "待执行指令区域";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(25, 4); cout << "输入方式";
		SetCoord(11, 5); cout << "待执行指令";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 16) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 19); cout << "设置 -> 输入方式：<键盘输入>";
		SetCoord(11, 3); cout << "输入方式为<键盘输入>时";
		SetCoord(11, 4); cout << "会首先提示输入将运行的总指令数";
		SetCoord(11, 5); cout << "后请逐行按语法输入执行指令";
		SetCoord(11, 6); cout << "到达输入行数后自动开始执行";
		SetCoord(14, 7); cout << "\\";
		SetCoord(66, 11); cout << "← 正确语法范例";
		SetCoord(66, 12); cout << "← 错误语法范例";
		SetCoord(66, 13); cout << "← 错误语法范例";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(25, 4); cout << "将运行的总指令数";
		SetCoord(30, 19); cout << "键盘输入";
		SetCoord(21, 3); cout << "<键盘输入>";
		SetCoord(15, 5); cout << "逐行按语法";
		SetCoord(25, 6); cout << "自动开始执行";
		SetCoord(66, 10); cout << "← 范围1~295";
		SetCoord(53, 25); cout << "页数(会自动翻页) →";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 17) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 19); cout << "设置 -> 输入方式：<文件输入>";
		SetCoord(11, 3); cout << "输入方式为<文件输入>时";
		SetCoord(11, 4); cout << "会首先提示输入读取的文件名/路径";
		SetCoord(11, 5); cout << "文件格式第一行为总指令数";
		SetCoord(11, 6); cout << "之后逐行为指令内容";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(31, 4); cout << "文件名/路径";
		SetCoord(30, 19); cout << "文件输入";
		SetCoord(21, 3); cout << "<文件输入>";
		SetCoord(19, 5); cout << "第一行为总指令数";
		SetCoord(15, 6); cout << "逐行为指令内容";
		SetCoord(63, 10); cout << "← [要求输入<1行]";
		SetCoord(53, 25); cout << "输入完成将自动读取并执行";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 18) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 3); cout << "然而，这两种输入方式都不算便捷";
		SetCoord(11, 4); cout << "在此，";
		SetCoord(11, 5); cout << "我们默认使用自创的第三种方式";
		SetCoord(11, 6); cout << "<光标输入>";
		SetCoord(14, 7); cout << "\\";
		SetCoord(51, 0); cout << "+--------------------------+";
		for (int i = 1; i < 28; i++) { SetCoord(51, i); cout << "|                          |"; }
		SetCoord(51, 28); cout << "+--------------------------+";
		SetCoord(53, 14); cout << "此区域均在光标移动范围内";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(12, 6); cout << "光标输入";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 19) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 19); cout << "设置 -> 输入方式：<光标输入>";
		SetCoord(11, 3); cout << "输入方式为<光标输入>时";
		SetCoord(11, 4); cout << "操作模式与主界面完全一致";
		SetCoord(11, 5); cout << "通过方向键选择当前选中的内容";
		SetCoord(11, 6); cout << "并按Enter进行选择/更改";
		SetCoord(14, 7); cout << "\\";
		SetCoord(43, 1); cout << "光标 →";
		SetCoord(62, 1); cout << "← Enter可添加指令";
		SetCoord(69, 2); cout << "至当前末尾";
		SetCoord(51, 1); cout << ">";
		SetCoord(32, 2); cout << "可跨区域移动/选择";
		SetCoord(48, 3); cout << "↓";
		SetCoord(52, 10); cout << "[Q/E]可切换页面 →";
		SetCoord(69, 12); cout << "← 指令参数";
		SetCoord(52, 16); cout << "↑";
		SetCoord(52, 17); cout << "当光标在某条已输入指令上时";
		SetCoord(52, 18); cout << "可以使用Enter进入二级菜单";
		SetCoord(52, 19); cout << "再使用左右键修改参数";
		SetCoord(52, 20); cout << "再次键入Enter可选定参数";
		SetCoord(52, 25); cout << "[当前页数/总页数] →";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(19, 4); cout << "与主界面完全一致";
		SetCoord(30, 19); cout << "光标输入";
		SetCoord(21, 3); cout << "<光标输入>";
		SetCoord(15, 5); cout << "方向键";
		SetCoord(15, 6); cout << "Enter";
		SetCoord(34, 2); cout << "跨区域";
		SetCoord(64, 1); cout << "Enter";
		SetCoord(52, 10); cout << "[Q/E]";
		SetCoord(60, 18); cout << "Enter";
		SetCoord(60, 20); cout << "Enter";
		SetCoord(58, 19); cout << "左右键";
		SetCoord(52, 22); cout << "提示：jump/jumpifzero";
		SetCoord(52, 23); cout << "只能指向已存在的指令";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 20) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 19); cout << "设置 -> 输入方式：<光标输入>";
		SetCoord(11, 3); cout << "当然，最下方还有三个功能按键";
		SetCoord(11, 4); cout << "具体功能如右所示";
		SetCoord(11, 5); cout << "你也可以随时通过Esc返回主菜单";
		SetCoord(11, 6); cout << "[此功能仅<光标输入>模式可用]";
		SetCoord(14, 7); cout << "\\";
		SetCoord(57, 26); cout << "↓        ↓        ↓";
		SetCoord(53, 24); cout << "停止编辑     |        |";
		SetCoord(53, 25); cout << "开始运行     |        |";
		SetCoord(53, 22); cout << "删除最后一条指令      |";
		SetCoord(66, 23); cout << "|        |";
		SetCoord(75, 21); cout << "|";
		SetCoord(53, 20); cout << "与Esc功能相同，返回主界面";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(30, 19); cout << "光标输入";
		SetCoord(27, 3); cout << "三个功能按键";
		SetCoord(27, 5); cout << "Esc";
		SetCoord(53, 25); cout << "开始运行";
		SetCoord(57, 22); cout << "最后一条";
		SetCoord(55, 20); cout << "Esc";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 21) {
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 19); cout << "设置 -> 单步跟踪：<开>";
		SetCoord(11, 5); cout << "程序开始了！";
		SetCoord(11, 6); cout << "你可以随时用Z/X键来让我减/加速";
		SetCoord(53, 22); cout << "如果你选择了单步跟踪";
		SetCoord(53, 23); cout << "每次操作后Enter以继续执行";
		SetCoord(57, 24); cout << "|";
		SetCoord(57, 25); cout << "|";
		SetCoord(57, 26); cout << "|";
		SetCoord(57, 27); cout << "↓";
		SetCoord(14, 7); cout << "\\";
		SetCoord(56, 28); cout << "回车以单步调试";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(23, 6); cout << "Z/X";
		SetCoord(34, 6); cout << "减/加速";
		SetCoord(65, 22); cout << "单步跟踪";
		SetCoord(63, 23); cout << "Enter";
		SetCoord(30, 19); cout << "开";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 22) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		print_message_box(25, 10, 54, 20, 1);
		SetCoord(29, 12);
		cout << "ABORTION";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		SetCoord(29, 14);
		cout << "放弃，是一种境界";
		SetCoord(29, 18);
		cout << ">重新开始        ·退出";
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 5); cout << "发现哪里写错了？";
		SetCoord(11, 6); cout << "你可以随时用Esc键来中断运行";
		SetCoord(14, 7); cout << "\\";
		SetCoord(12, 18); cout << "左右键切换 →";
		SetCoord(12, 19); cout << "Enter键选择";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(23, 6); cout << "Esc";
		SetCoord(12, 18); cout << "左右键";
		SetCoord(12, 19); cout << "Enter";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 23) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		print_message_box(25, 10, 54, 20, 1);
		SetCoord(29, 12);
		cout << "ERROR";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		SetCoord(29, 18);
		cout << ">重新开始        ·退出";
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 4); cout << "运行出错？答案错误？";
		SetCoord(11, 5); cout << "你可以选择重新开始，";
		SetCoord(11, 6); cout << "直接在上次代码的基础上做修改";
		SetCoord(14, 7); cout << "\\";
		SetCoord(12, 18); cout << "左右键切换 →";
		SetCoord(12, 19); cout << "Enter键选择";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(15, 6); cout << "在上次代码的基础上做修改";
		SetCoord(12, 18); cout << "左右键";
		SetCoord(12, 19); cout << "Enter";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
		SetCoord(29, 16); cout << "报错详细信息";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 24) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		print_message_box(25, 10, 54, 20, 1);
		SetCoord(29, 12);
		cout << "SUCCESS";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		SetCoord(29, 14);
		cout << "指令行数：4";
		SetCoord(29, 16);
		cout << "总执行次数：5";
		SetCoord(29, 18);
		cout << "请按任意键继续. . .";
		SetCoord(43, 12);
		cout << "   得到：";
		SetCoord(45, 14);
		cout << "银勋章";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
		cout << "*";
		print_robot(16, 8, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(11, 4); cout << "恭喜通过本关！";
		SetCoord(11, 5); cout << "你可以在提示框内看到";
		SetCoord(11, 6); cout << "本次运行的数据与挑战达成情况";
		SetCoord(14, 7); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(15, 6); cout << "运行的数据";
		SetCoord(27, 6); cout << "挑战达成情况";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 25) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(13, 2); cout << "关卡不够玩？来尝试自创关卡替换，以下是关卡文件格式";
		SetCoord(64, 3); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(12, 12); cout << "[关卡文件格式]";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	if (page == 26) {
		print_robot(66, 4, 0, 0);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		SetCoord(25, 2); cout << "教程结束了，希望能在游戏中看到你，886~";
		SetCoord(64, 3); cout << "\\";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
		SetCoord(4, 21); cout << "你可以由这里进入再次观看教程 →";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
}
void guidance() {
	ifhelping = 1;
	system("cls");
	ifstream guid;
	int guide_pages = 1, maxpages = 26;
	string input_tmp;
	guid.open("guidance");
	while (getline(guid, input_tmp) && input_tmp != "@" + to_string(guide_pages)) {}
	for (int i = 0; i < 29; i++) {
		getline(guid, input_tmp);
		cout << input_tmp;
		if (i < 29) cout << endl;
	}
	guidance_assist(guide_pages);
	guid.close();
	while (1) {
		if (_kbhit()) {
			int tmp;
			tmp = _getch();
			if (tmp == 27) {
				system("cls");
				return;
			}
			if (tmp == int('q') || tmp == int('Q') || tmp == int('e') || tmp == int('E')) {
				if ((tmp == int('q') || tmp == int('Q')) && guide_pages == 1) continue;
				if ((tmp == int('q') || tmp == int('Q')) && guide_pages > 1) {
					system("cls");
					guide_pages--;
				}
				if ((tmp == int('e') || tmp == int('E')) && guide_pages == maxpages) continue;
				if ((tmp == int('e') || tmp == int('E')) && guide_pages < maxpages) {
					system("cls");
					guide_pages++;
				}
				guid.open("guidance");
				while (getline(guid, input_tmp) && input_tmp != "@" + to_string(guide_pages)) {}
				for (int i = 0; i < 29; i++) {
					getline(guid, input_tmp);
					cout << input_tmp;
					if (i < 29) cout << endl;
				}
				guidance_assist(guide_pages);
				guid.close();
			}
		}
	}
}//教程界面
void helping_UI() {
	//keybd_event(VK_CONTROL, 0, 0, 0);
	Sleep(80);
	SetCoord(36, 19);
	cout << "|>";
	SetCoord(42, 19);
	cout << " ";
	Sleep(80);
	SetCoord(35, 19);
	cout << "| ";
	Sleep(80);
	SetCoord(35, 20);
	cout << "|";
	Sleep(80);
	SetCoord(35, 21);
	cout << "| ·游玩教程";
	Sleep(80);
	SetCoord(35, 22);
	cout << "| ·关于";
	//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
	int pin = 0; // 0, 2, 3 对应不同的按键
	while (1) {
		if (_kbhit()) {//如果有按键按下，则_kbhit()函数返回真
			int temp;
			temp = _getch();//使用_getch()函数获取按下的键值
			if (temp == 27) {
				//keybd_event(VK_CONTROL, 0, 0, 0);
				Sleep(80);
				rect_clear(35, 22, 80, 29);
				Sleep(80);
				rect_clear(35, 21, 80, 29);
				Sleep(80);
				rect_clear(35, 20, 80, 29);
				Sleep(80);
				SetCoord(35, 19);
				cout << " |>";
				Sleep(80);
				SetCoord(36, 19);
				cout << ">|帮助|";
				//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
				break;
			}
			if (temp == 224) {
				SetCoord(37, 19 + pin);
				if (pin == 0) cout << ' ';
				else cout << "·";
				int temp2 = _getch();
				if (temp2 == 80 && pin < 3) {
					if (pin == 0) pin++;
					pin++;
				}
				else if (temp2 == 72 && pin > 0) {
					if (pin == 2) pin--;
					pin--;
				}
				SetCoord(37, 19 + pin);
				cout << '>';
			}
			if (temp == 13) {
				if (pin == 0) {
					//keybd_event(VK_CONTROL, 0, 0, 0);
					Sleep(80);
					rect_clear(35, 22, 80, 29);
					Sleep(80);
					rect_clear(35, 21, 80, 29);
					Sleep(80);
					rect_clear(35, 20, 80, 29);
					Sleep(80);
					SetCoord(35, 19);
					cout << " |>";
					Sleep(80);
					SetCoord(36, 19);
					cout << ">|帮助|";
					//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
					break;
				}
				if (pin == 2) {
					guidance();
					return;
				}
				if (pin == 3) {
					for_us();
					return;
				}
			}
		}
	}
}//帮助界面函数
void exit_UI () {
	SetCoord(48, 21);
	cout << "您是否确定要退出？";
	SetCoord(48, 22);
	cout << "按Y/Enter确认退出，按N/Esc取消";
	while (1) {
		if (_kbhit()) {
			int temp_e;
			temp_e = _getch();
			if (temp_e == 'Y' || temp_e == 'y' || temp_e == 13) exit(0);
			if (temp_e == 'N' || temp_e == 'n' || temp_e == 27) break;
		}
	}
	rect_clear(48, 21, 79, 28);
	return;
}
//页面初始化

string shrink_space(string raw_para) {
	int tmp = 0;
	for (int i = 0; i < raw_para.length(); i++) {
		if (raw_para[i] == ' ') tmp++;
		else break;
	}
	raw_para = raw_para.substr(tmp, raw_para.length());
	tmp = 0;
	for (int i = raw_para.length() - 1; i >= 0; i--) {
		if (raw_para[i] == ' ') tmp++;
		else break;
	}
	raw_para = raw_para.substr(0, raw_para.length() - tmp);
	return raw_para;
}//删减前后空格
bool para_check(string para_tmp) {
	bool ifisdigit = true;
	for (int i = 0; i < para_tmp.length(); i++)
		if (para_tmp[i] - '0' < 0 || para_tmp[i] - '9' > 0)
			ifisdigit = false;
	if (para_tmp.length() > 1 && para_tmp[0] == '-') ifisdigit = true;
	return ifisdigit;
}
int interpret_digit(string para_tmp) {
	int sum = 0;
	for (int i = para_tmp.length() - 1; i > 0; i--) sum += (para_tmp[i] - '0') * pow(10, para_tmp.length() - i - 1);
	if (para_tmp[0] == '-') sum *= -1;
	else sum += (para_tmp[0] - '0') * pow(10, para_tmp.length() - 1);
	return sum;
}
//字符串操作模块
void abort(int code) {
	abortion = true;
	ending_UI(code);
	return;
	// malfunction code:
	// 0: command unusable
	// 1: holding air
	// 2: memory out of stretch
	// 3: memory empty
	// 4: command not exist (only occur in jump & jumpifzero) 
	// 5: wrong answer
	// 6: abortion
	// 7: command number invalid
	// 8: command number unmatched
	// 9: empty file
	// 10: no file
	// 11：number exceeded
	// 12: death loop
	// 13：file name length invalid
	// -1：input error
	// -2: input out of limit
} // wait to refine, the only input to this function is malfunction code
void succeed() {
	winning = true;
	SetCoord(41, 28);
	cout << cmd_cnt << "/" << achievement_array[chosen_level][2] << "   ";
	ending_UI(0);
	return;
} // wait to refine////  
//结束中继模块

void reset() {
	if_holding = 0, holding_tmp = 0;
	memset(output_array, 0, sizeof(output_array));
	memset(memory, 0, sizeof(memory));
	memset(if_memory_use, 0, sizeof(if_memory_use));
	input_tmp = 0, output_tmp = 0;
	cmd_tmp = 1; last_cmd_tmp = 0; cmd_cnt = 0; abortion = 0; winning = 0;
	speed = 100, speed_sign = 2, speed_pause_tmp = 0;
	ifrestarting = 1;
	//全局变量置零
}
void reset_cmd() {
	for (int i = 0; i <= 296; i++) {
		cmd_array[i] = "";
		cmd_para[i] = "";
	}
}

void read_cmd1() {
	ReviveConsoleCursor();
	cmd_sum = 0;
	SetCoord(51, 9);
	cout << "=========待执行指令=========";
	SetCoord(60, 27);
	cout << "输入中...";
	SetCoord(52, 10);
	cout << "指令数量为：";
	string cmd_sum_tmp;
	int cmd_sum_data = 0;
	getline(cin, cmd_sum_tmp);
	if (cmd_sum_tmp == "") {
		abort(7);
		set_ready = 0;
		return;
	}
	cmd_sum_tmp = shrink_space(cmd_sum_tmp);
	if (para_check(cmd_sum_tmp)) cmd_sum_data = interpret_digit(cmd_sum_tmp);
	else {
		abort(7);
		set_ready = 0;
		return;
	}
	if (cmd_sum_data > 295 || cmd_sum_data <= 0) {
		abort(7);
		set_ready = 0;
		return;
	}
	for (int j = 1; j <= ( cmd_sum_data - 1 ) / 15 + 1; j++) {
		pages = j;
		page_refresh(j);
		for (int i = (j - 1) * 15 + 1; i <= min(cmd_sum_data, j * 15); i++) {
			SetCoord(52, 10 + i - (j - 1) * 15);
			cout << i << ' ';
			string temp;
			int pause;
			getline(cin, temp);
			if (temp.find(" ") != string::npos) {
				pause = temp.find(" ");
				cmd_array[i] = temp.substr(0, pause);
				cmd_para[i] = temp.substr(pause + 1, 20);
			}
			else {
				cmd_array[i] = temp;
				cmd_para[i] = "";
			}
			cmd_sum++;
		}
	}
}//键盘指令读取
void read_cmd2() {
	cmd_sum = 0;
	string filename;
	ifstream cmd;
	SetCoord(51, 9);
	cout << "=========待执行指令=========";
	SetCoord(60, 27);
	cout << "输入中...";
	SetCoord(52, 10);
	cout << "指令文件为：";
	ReviveConsoleCursor();
	int cur_cmd_num = 1;
	getline(cin, filename);
	HideConsoleCursor();
	
	if (filename.length() > 15 || filename == "") {
		abort(13);
		set_ready = 0;
		return;
	}

	SetCoord(52, 10);
	cout << "                           ";
	cmd.open(filename);
	if (cmd) {
		string cmd_sum_tmp;
		cmd >> cmd_sum_tmp;
		if (cmd_sum_tmp == "") {
			abort(7);
			set_ready = 0;
			return;
		}
		cmd_sum_tmp = shrink_space(cmd_sum_tmp);
		if (para_check(cmd_sum_tmp)) cmd_sum = interpret_digit(cmd_sum_tmp);
		else {
			abort(7);
			set_ready = 0;
			return;
		}
		if (cmd_sum > 295 || cmd_sum <= 0) {
			abort(7);
			set_ready = 0;
			return;
		}
		SetCoord(52, 10);
		cout << "指令数量为:" << cmd_sum;
		if (cmd.eof()) {
			abort(9);
			set_ready = 0;
			return;
		}
		cmd.seekg(0, ios_base::beg);//判断是否空文件并把文件指针移回开头
		cmd.ignore(1024, '\n');
		while (!cmd.eof()) {
			string temp;
			int pause;
			getline(cmd, temp);
			//SetCoord(52, 10 + cur_cmd_num);
			//cout << cur_cmd_num << ' ' << temp << endl;
			if (temp.find(" ") != string::npos) {
				pause = temp.find(" ");
				cmd_array[cur_cmd_num] = temp.substr(0, pause);
				cmd_para[cur_cmd_num] = temp.substr(pause + 1, 20);
			}
			else {
				cmd_array[cur_cmd_num] = temp;
				cmd_para[cur_cmd_num] = "";
			}
			cur_cmd_num++;
			pages = ( cmd_sum - 1 ) / 15 + 1;
			page_refresh(1);
		}
		if (cur_cmd_num != cmd_sum + 1) {
			abort(8);
			set_ready = 0;
			return;
		}
	}//暂不能处理空文件
	else {
		abort(10);
		set_ready = 0;
		return;
	}
	cmd.close();
}//文件指令读取
void read_cmd3() {
	SetCoord(51, 9);
	cout << "=========待执行指令=========";
	SetCoord(54, 27);
	cout << "|开始|   |回退|   |退出|";
	page_refresh_special(current_page);
	read_pin = 1;
	// cur_num -> cmd_sum - (pages - 1) * 15 可以转化为当前的行数
	//int cur_num = 0;//已读入指令数
	SetCoord(51, 1);
	cout << '>';
	SetCoord(70, 10);
	cout << "[Q/E翻页]";
	int tar_tmp = 0;
	while (1) {
		if (_kbhit()) {
			if (cmd_sum > 295) {
				set_ready = 0;
				available_num = 0, read_pin = 1, current_page = 1, pages = 1, cmd_sum = 0;
				memset(input_array, 0, sizeof(input_array));
				memset(expected_array, 0, sizeof(expected_array));
				memset(usable, 0, sizeof(usable));
				memset(paratemp, -1, sizeof(paratemp));
				reset();
				reset_cmd();
				abort(-2);
				return;
			}
			int temp;
			temp = _getch();
			if (temp == 27) {
				set_ready = 0;
				ifrestarting = 0;
				level_end(chosen_level);
				return;
			}
			if (temp == int('q') || temp == int('Q')) {
				if (current_page > 1) {
					current_page--;
					page_refresh_special(current_page);
					if (read_pin >= 11 && read_pin <= 25) {
						SetCoord(51, read_pin);
						cout << ">";
					}
				}
			}
			if (temp == int('e') || temp == int('E')) {
				if (current_page < pages) {
					current_page++;
					page_refresh_special(current_page);
					if (current_page == pages) read_pin = min(read_pin, 10 + cmd_sum - (pages - 1) * 15);
					if (read_pin >= 11 && read_pin <= 25) {
						SetCoord(51, read_pin);
						cout << ">";
					}
				}
			}
			tar_tmp = min(current_page * 15, cmd_sum);
			if (temp == 224) {
				int temp2 = _getch();
				if (read_pin > 0) {
					SetCoord(51, read_pin);
					cout << ' ';
				}
				else {
					SetCoord(80 + 9 * read_pin, 27);
					cout << ' ';
				}
				if (temp2 == 80 && read_pin < 10 + tar_tmp - (current_page - 1) * 15 && read_pin != available_num - 1 && read_pin >= 0) read_pin++;
				else if (temp2 == 72 && read_pin > 1 && read_pin != 11 && read_pin != 10) read_pin--;
				else if (temp2 == 80 && read_pin == available_num - 1) {
					if (tar_tmp - (current_page - 1) * 15 != 0) read_pin = 11;
					else read_pin = -3;
				}
				else if (temp2 == 72 && (read_pin == 11 || read_pin == 10)) read_pin = available_num - 1;
				else if (temp2 == 80 && read_pin == 10 + tar_tmp - (current_page - 1) * 15) read_pin = -3;
				else if (temp2 == 72 && read_pin < 0) {
					if (cmd_sum == 0) read_pin = available_num - 1;
					else read_pin = 10 + tar_tmp - (current_page - 1) * 15;
				}
				else if (temp2 == 77 && read_pin<-1 && read_pin>-4) read_pin++;
				else if (temp2 == 75 && read_pin<0 && read_pin>-3) read_pin--;
				if (read_pin > 0) {
					SetCoord(51, read_pin);
					cout << '>';
				}
				else {
					SetCoord(80 + 9 * read_pin, 27);
					cout << '>';
				}
			}//用方向键移动光标
			if (temp == 13 && read_pin <= 8 && read_pin >= 0) {
				page_refresh_special(pages);
				current_page = pages;
				cmd_sum++;
				if (cmd_sum == pages * 15 + 1) {
					pages++;
					page_refresh_special(pages);
					current_page = pages;
				}
				tar_tmp = min(current_page * 15, cmd_sum);
				if (usable[read_pin] == 1) cmd_array[cmd_sum] = "inbox";
				if (usable[read_pin] == 2) cmd_array[cmd_sum] = "outbox";
				if (usable[read_pin] == 3) cmd_array[cmd_sum] = "add";
				if (usable[read_pin] == 4) cmd_array[cmd_sum] = "sub";
				if (usable[read_pin] == 5) cmd_array[cmd_sum] = "copyto";
				if (usable[read_pin] == 6) cmd_array[cmd_sum] = "copyfrom";
				if (usable[read_pin] == 7) cmd_array[cmd_sum] = "jump";
				if (usable[read_pin] == 8) cmd_array[cmd_sum] = "jumpifzero";
				SetCoord(52, 10 + tar_tmp - (current_page - 1) * 15);
				cout << cmd_sum << ' ' << cmd_array[cmd_sum];

				if (usable[read_pin] >= 7) {
					paratemp[cmd_sum] = 1;
					SetCoord(67, 10 + tar_tmp - (current_page - 1) * 15);
					cout << 1;
				}
				else if (usable[read_pin] >= 3 && usable[read_pin] < 7) {
					paratemp[cmd_sum] = 0;
					SetCoord(67, 10 + tar_tmp - (current_page - 1) * 15);
					cout << 0;
				}
				else {
					paratemp[cmd_sum] = -1;
				}
				if (cmd_sum % 15 == 1) page_refresh_special(pages);
			}//在可用指令列表中键入enter，则在待执行列表中添加该指令，若有操作数，操作数默认设为1
			if (temp == 13 && read_pin > 8) {
				int line = read_pin - 10; // line -> line + (current_page - 1) * 15 还原当前行数
				int uplimit = ready_RAM - 1; int downlimit = 0;
				if (cmd_array[line + (current_page - 1) * 15] == "inbox" || cmd_array[line + (current_page - 1) * 15] == "outbox" || cmd_array[line + (current_page - 1) * 15] == "") continue;
				if (cmd_array[line + (current_page - 1) * 15] == "jump" || cmd_array[line + (current_page - 1) * 15] == "jumpifzero") { uplimit = cmd_sum; downlimit = 1; }
				int num_length_tmp = 1, num_para_tmp = paratemp[line + (current_page - 1) * 15];
				while (num_para_tmp / 10) {
					num_para_tmp /= 10;
					num_length_tmp++;
				}
				SetCoord(66, read_pin);
				cout << '<';
				SetCoord(67 + num_length_tmp, read_pin);
				cout << ">  ";
				while (1) {
					if (_kbhit()) {
						int temp;
						temp = _getch();
						if (temp == 224) {
							int temp2 = _getch();
							if (temp2 == 77 && paratemp[line + (current_page - 1) * 15] < uplimit) paratemp[line + (current_page - 1) * 15]++;
							else if (temp2 == 75 && paratemp[line + (current_page - 1) * 15] > downlimit) paratemp[line + (current_page - 1) * 15]--;
							SetCoord(67, read_pin);
							cout << paratemp[line + (current_page - 1) * 15] << ">  ";
						}//左右键调整操作数，操作数上下限由程序计算，因此输入不会超出上下限
						if (temp == 13) break;
					}
				}
				SetCoord(66, read_pin);
				cout << ' ';
				num_length_tmp = 1, num_para_tmp = paratemp[line + (current_page - 1) * 15];
				while (num_para_tmp / 10) {
					num_para_tmp /= 10;
					num_length_tmp++;
				}
				SetCoord(67 + num_length_tmp, read_pin);
				cout << "   ";
			}//在待执行列表中键入enter，则开始用左右键调整该行操作数（若无操作数程序不会响应）
			if (temp == 13 && read_pin == -3) {
				for (int i = 1; i <= cmd_sum; i++) {
					if (paratemp[i] < 0) cmd_para[i] = "";
					else  cmd_para[i] = to_string(paratemp[i]);
				}
				SetCoord(54, 27);
				cout << "                          ";
				page_refresh_special(1);
				SetCoord(70, 10);
				cout << "         ";
				return;
			}//在“开始”选项处键入enter，开始执行程序
			if (temp == 13 && read_pin == -1) {
				set_ready = 0;
				ifrestarting = 0;
				level_end(chosen_level);
				return;
			}//在“退出”选项处键入enter，退到主菜单
			if (temp == 13 && read_pin == -2) {
				if (cmd_sum == 0) continue;
				page_refresh_special(pages);
				if (cmd_sum == (pages - 1) * 15 + 1 && cmd_sum != 1) {
					cmd_array[cmd_sum] = "";
					cmd_sum--;
					pages--;
					page_refresh_special(pages);
					current_page = pages;
				}
				else {
					SetCoord(51, 10 + cmd_sum - (pages - 1) * 15);
					cout << "                  ";
					cmd_array[cmd_sum] = "";
					cmd_sum--;
				}
				tar_tmp = min(current_page * 15, cmd_sum);
			}//在“回退”选项处键入enter，清除上一条输入的指令
		}
	}
}//方向键光标指令读取
void cmd_pin() {
	SetCoord(51, 10 + (last_cmd_tmp - 1) % 15 + 1);
	cout << ' ';
	SetCoord(51, 10 + (cmd_tmp - 1) % 15 + 1);
	cout << '>';
	last_cmd_tmp = cmd_tmp;
}//执行中的指令光标
//以上函数负责指令输入及显示





void output_check() {
	SetCoord(41, 28);
	cout << cmd_cnt << "/" << achievement_array[chosen_level][2] << "   ";
	if (output_array[output_tmp] != expected_array[output_tmp]) { abort(5); return; }
	if (output_tmp == ans_sum) succeed();
	return;
}
void inbox() {
	if (!if_usable[1]) { abort(0); return; } // inbox指令不可用; 报错0
	robot_animat(0); // 机器人移动到最左侧
	if (input_tmp == input_sum) {
		output_check();
		if (output_tmp != ans_sum) abort(5);
		return;
	} // 无块可拿
	cmd_cnt++; // 执行指令计数
	inbox_renew_v1(); // 机器人拿取最左侧块
	if_holding = true;
	input_tmp++;
	holding_tmp = input_array[input_tmp]; // 更新输入序列和手中块信息
	return;
}
void outbox() {
	if (!if_usable[2]) { abort(0); return; } // outbox指令不可用; 报错0
	robot_animat(5); // 机器人移动到最右侧
	if (!if_holding) { abort(1); return; } // 手中为空; 报错1
	cmd_cnt++; // 执行指令计数
	outbox_renew_v1(); // 机器人放置最右侧块
	if_holding = false;
	output_tmp++;
	output_array[output_tmp] = holding_tmp; // 更新输出序列和手中块信息
	output_check(); // 每次输出时检测输出是否正确
	return;
}
void add(int add_pos) {
	if (!if_usable[3]) { abort(0); return; } // add指令不可用; 报错0
	if (add_pos < 0 || add_pos >= ready_RAM) { abort(2); return; } // 内存溢出; 报错2
	robot_animat(add_pos + 1); // 机器人移动到指定位置
	if (!if_holding) { abort(1); return; } // 手中为空; 报错1
	if (!if_memory_use[add_pos]) { abort(3); return; } // 内存为空; 报错3
	if (holding_tmp + memory[add_pos] > 999 || holding_tmp + memory[add_pos] < -99) { abort(11); return; } // 数据溢出；报错11
	holding_tmp += memory[add_pos]; // 更新手中块数值
	cmd_cnt++; // 执行指令计数
	RAM_renew(add_pos); // 更新显示
	return;
}
void sub(int sub_pos) {
	if (!if_usable[4]) { abort(0); return; } // sub指令不可用; 报错0
	if (sub_pos < 0 || sub_pos >= ready_RAM) { abort(2); return; } // 内存溢出; 报错2
	robot_animat(sub_pos + 1); // 机器人移动到指定位置
	if (!if_holding) { abort(1); return; } // 手中为空; 报错1
	if (!if_memory_use[sub_pos]) { abort(3); return; } // 内存为空; 报错3
	if (holding_tmp - memory[sub_pos] > 999 || holding_tmp + memory[sub_pos] < -99) { abort(11); return; } // 数据溢出；报错11
	holding_tmp -= memory[sub_pos]; // 更新手中块数值
	cmd_cnt++; // 执行指令计数
	RAM_renew(sub_pos); // 更新内存和手中块显示
	return;
}
void copyto(int copyto_pos) {
	if (!if_usable[5]) { abort(0); return; } // copyto指令不可用; 报错0
	if (copyto_pos < 0 || copyto_pos >= ready_RAM) { abort(2); return; } // 内存溢出; 报错2
	robot_animat(copyto_pos + 1); // 机器人移动到指定位置
	if (!if_holding) { abort(1); return; } // 手中为空; 报错1
	if_memory_use[copyto_pos] = true;
	memory[copyto_pos] = holding_tmp; // 更新内存信息
	cmd_cnt++; // 执行指令计数
	RAM_renew(copyto_pos);
	return;
}
void copyfrom(int copyfrom_pos) {
	if (!if_usable[6]) { abort(0); return; } // copyfrom指令不可用; 报错0
	if (copyfrom_pos < 0 || copyfrom_pos >= ready_RAM) { abort(2); return; } // 内存溢出; 报错2
	robot_animat(copyfrom_pos + 1); // 机器人移动到指定位置
	if (!if_memory_use[copyfrom_pos]) { abort(3); return; } // 内存为空; 报错3
	if_holding = true;
	holding_tmp = memory[copyfrom_pos]; // 更新手中块信息
	cmd_cnt++; // 执行指令计数
	RAM_renew(copyfrom_pos);
	return;
}
void jump(int jump_to) {
	if (!if_usable[7]) { abort(0); return; } // jump指令不可用; 报错0
	if (jump_to <= 0 || jump_to > cmd_sum) { abort(4); return; } // 指向指令不存在; 报错4
	Sleep(speed * 5);
	cmd_tmp = jump_to - 1; // 使得在通用+1后指针达到既定位置
	cmd_cnt++; // 执行指令计数
	return;
}
void jumpifzero(int jumpifzero_to) {
	if (!if_usable[8]) { abort(0); return; } // jumpifzero指令不可用; 报错0
	if (jumpifzero_to <= 0 || jumpifzero_to > cmd_sum) { abort(4); return; } // 指向指令不存在; 报错4
	Sleep(speed * 5);
	if (!if_holding) { abort(1); return; } // 手中为空; 报错1
	if (holding_tmp == 0) cmd_tmp = jumpifzero_to - 1;
	cmd_cnt++; // 执行指令计数
	return;
}
//以上函数负责指令执行

void thread_speed() {
	//while (!abortion && !winning && !step_trace) {
	while (!abortion && !winning) {
		if (_kbhit()) {
			speed_key_detect = _getch();
			if (speed_key_detect == int('x') && speed_sign < 128) {
				speed_sign *= 2;
				speed = 200 / speed_sign;
			}
			if (speed_key_detect == int('z') && speed_sign > 1) {
				speed_sign /= 2;
				speed = 200 / speed_sign;
			}
			if (speed_key_detect == 27) abort(6);
		}
		else speed_key_detect = '0';
		Sleep(2);
	}
} // z -> slow; x -> fast; esc -> exit ( abortion code = 6 )

void execution() {
	string shrinked;
	SetCoord(12, 28);
	cout << cmd_sum << "/" << achievement_array[chosen_level][1] << "   ";
	if (cmd_sum > 295 || cmd_sum <= 0) {
		abort(7);
		set_ready = 0;
		return;
	}
	while (!abortion && !winning) { // 检测当前是否终止

		if (cmd_cnt > 300) {
			abort(12);
			return;
		}

		if (step_trace) {
			while (true) {
				if (_kbhit()) {
					int key_tmp = _getch();
					if (key_tmp == 27) {
						abort(6);
						break;
					} // esc
					if (key_tmp == int('x') && speed_sign < 128) {
						speed_sign *= 2;
						speed = 200 / speed_sign;
					}
					if (key_tmp == int('z') && speed_sign > 1) {
						speed_sign /= 2;
						speed = 200 / speed_sign;
					}
					if (key_tmp == 13) break; // enter
				}
			}
			if (abortion) break;
		}
		Sleep(20);

		if (input_mode == 2) page_refresh_special((cmd_tmp - 1) / 15 + 1);
		else page_refresh((cmd_tmp - 1) / 15 + 1);

		cmd_pin(); // 更新当前指令位置
		shrinked = shrink_space(cmd_para[cmd_tmp]);
		if (cmd_array[cmd_tmp] == "inbox" && shrinked == "") inbox();
		else if (cmd_array[cmd_tmp] == "outbox" && shrinked == "") outbox();
		else if (cmd_array[cmd_tmp] == "add" && shrinked != "") {
			if (!para_check(shrinked)) { abort(-1); return; } // 参数不合法; 报错-1
			else add(interpret_digit(shrinked)); // 参数由字符串转int后调用相应操作函数
		}
		else if (cmd_array[cmd_tmp] == "sub" && shrinked != "") {
			if (!para_check(shrinked)) { abort(-1); return; }
			else sub(interpret_digit(shrinked));
		}
		else if (cmd_array[cmd_tmp] == "copyto" && shrinked != "") {
			if (!para_check(shrinked)) { abort(-1); return; }
			else copyto(interpret_digit(shrinked));
		}
		else if (cmd_array[cmd_tmp] == "copyfrom" && shrinked != "") {
			if (!para_check(shrinked)) { abort(-1); return; }
			else copyfrom(interpret_digit(shrinked));
		}
		else if (cmd_array[cmd_tmp] == "jump" && shrinked != "") {
			if (!para_check(shrinked)) { abort(-1); return; }
			else jump(interpret_digit(shrinked));
		}
		else if (cmd_array[cmd_tmp] == "jumpifzero" && shrinked != "") {
			if (!para_check(shrinked)) { abort(-1); return; }
			else jumpifzero(interpret_digit(shrinked));
		}
		else { abort(-1); return; } // 输入命令不合法; 报错-1
		if (winning || abortion) break; // 若已结束则跳出

		SetCoord(41, 28);
		cout << cmd_cnt << "/" << achievement_array[chosen_level][2] << "   ";

		if (cmd_tmp < cmd_sum) cmd_tmp++; // 进入下一条命令
		else {
			output_check();
			if (output_tmp != ans_sum) abort(5);
			return;
		} // 命令执行完毕时检查答案
	}
	return;
}
//指令执行核心函数

int main() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX fontInfo = { sizeof(CONSOLE_FONT_INFOEX) };
	GetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
	fontInfo.dwFontSize.X = 20;
	fontInfo.dwFontSize.Y = 20;
	SetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
	setsize(81, 30);
	HideConsoleCursor();

	set_level_gui();
	set_menu_gui();

	firstrunning.open("fr.tmp");
	if (!firstrunning) {
		firstrunning.close();
		firstrunning_in.open("fr.tmp");
		PlaySound(TEXT("menu.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
		guidance();
	}
	else firstrunning.close();
	//首次运行进入教程

	game_initiate_animat(); // 开场动画渲染

	//cmd界面初始化
	while (1) {
		save.open("save.save");
		bool ifallpassed = 0, ifallachieved = 1;
		if (save) {
			save >> passed_level;
			if (passed_level == 5) ifallpassed = 1;
			for (int i = 1; i <= 5; i++) {
				save >> achievements[i][1] >> achievements[i][2];
				if (achievements[i][1] + achievements[i][2] != 2) ifallachieved = 0;
			}
		}
		else {
			passed_level = 0;
			for (int i = 1; i <= 5; i++) {
				achievements[i][1] = 0;
				achievements[i][2] = 0;
			}
		}
		save.close();
		//存档文件读取

		user_pref.open("preferences");
		if (user_pref) {
			user_pref >> input_mode >> voices >> step_trace;
		}
		else {
			input_mode = 2;
			voices = 1;
			step_trace = 0;
		}
		user_pref.close();
		//用户偏好读取

		if (ifresetting || !ifhelping || ifforus) {
			PlaySound(NULL, NULL, SND_PURGE); //背景音乐重置
			if (voices) {
				PlaySound(TEXT("menu.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
			}//背景音乐启动
		}

		ifresetting = 0, ifhelping = 0, ifforus = 0;

		menu_init();
		if (ifallpassed) {
			if (ifallachieved) {
				SetCoord(29, 15);
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				cout << "恭喜通关！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN);
				cout << "[全成就达成]";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
			else {
				SetCoord(35, 15);
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				cout << "恭喜通关！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
		}
		chosen_level = 1;//当前光标选中的关卡（11，12，13不为关卡，而是特殊选项）
		menu_pin(1, 1);
		while (1) {
			if (ifresetting || ifhelping || ifforus) break;
			while (1) {
				if (_kbhit()) {//如果有按键按下，则_kbhit()函数返回真
					int temp;
					temp = _getch();//使用_getch()函数获取按下的键值
					if (temp == 13) break;
					if (temp == 27) {
						exit_UI();
						continue;
					}
					if (temp == 224) {
						int temp2 = _getch();
						menu_pin(chosen_level, 0);
						if (temp2 == 77 && chosen_level != 5 && chosen_level != 13) chosen_level++;
						else if (temp2 == 75 && chosen_level != 1 && chosen_level != 11) chosen_level--;
						else if (temp2 == 80 && chosen_level < 3) chosen_level = 11;
						else if (temp2 == 80 && chosen_level == 3) chosen_level = 12;
						else if (temp2 == 80 && chosen_level > 3) chosen_level = 13;
						else if (temp2 == 72 && chosen_level >= 11) chosen_level = 2 * chosen_level - 21;
						menu_pin(chosen_level, 1);
						continue;
					}
				}
			}
			if (chosen_level == 11) {
				setting_UI();
				if (!ifresetting) {
					user_pref_in.open("preferences");
					user_pref_in << input_mode << " " << voices << " " << step_trace;
					user_pref_in.close();
				}
				continue;
			}//设置界面
			if (chosen_level == 12) {
				helping_UI();
				continue;
			}//帮助界面
			if (chosen_level == 13) {
				exit_UI();
				continue;
			}//退出游戏
			if (chosen_level > passed_level + 1) {
				keybd_event(VK_CONTROL, 0, 0, 0);
				locked_level_animat(chosen_level);
				keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
				continue;
			}//判断关卡解锁情况
			else {
				string temp;
				temp = to_string(chosen_level);
				string level_name = "level_" + temp + ".txt";
				level_init.open(level_name);
				level_chosen_complete(chosen_level);
				break;
			}//关卡已解锁，读入关卡配置文件
		}
		//主界面循环
		if (ifresetting || ifhelping || ifforus) continue;
		//重置游戏界面与重加载特判

		PlaySound(NULL, NULL, SND_PURGE); //背景音乐重置
		if (voices) {
			PlaySound(TEXT("playing.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
		}//关卡音乐启动

		level_init >> input_sum >> ans_sum >> ready_RAM;
		for (int i = 1; i <= input_sum; i++) level_init >> input_array[i];
		for (int i = 1; i <= ans_sum; i++) level_init >> expected_array[i];
		for (int i = 1; i <= 8; i++) level_init >> if_usable[i];
		level_init >> achievement_array[chosen_level][1] >> achievement_array[chosen_level][2]; // 成就要求读入
		level_init >> instruction;
		level_init.close();//关卡配置文件读入

		while (ifrestarting) {
			system("cls");
			gaming_init();
			SetCoord(0, 25);
			int instruct_line = 0;
			for (int i = 0; i < instruction.length(); i++) {
				if (i % 50 == 0 && i != 0) {
					instruct_line++;
					SetCoord(0, 25 + instruct_line);
				}
				cout << instruction[i];
			}//在给定位置输出关卡任务要求
			SetCoord(0, 28);
			cout << "指令数挑战: " << achievement_array[chosen_level][1];
			SetCoord(29, 28);
			cout << "执行数挑战: " << achievement_array[chosen_level][2];
			set_ready = 1;

			if (input_mode == 0) {
				cmd_sum = 0;
				reset();
				reset_cmd();
				read_cmd1();
			}
			if (input_mode == 1) {
				cmd_sum = 0;
				reset();
				reset_cmd();
				read_cmd2();
			}
			if (input_mode == 2) {
				reset();
				read_cmd3();
			}
			HideConsoleCursor();
			if (set_ready == 1) {
				SetCoord(60, 27);
				cout << "执行中...";
				thread speeding(thread_speed);
				if (step_trace) {
					SetCoord(56, 28);
					cout << "回车以单步调试";
				}
				execution();
				speeding.join();
			}
			//若指令读入成功，开始执行玩家指令
		}
		//判断指令读取方式

		available_num = 0;
		read_pin = 1;
		current_page = 1;
		pages = 1;
		cmd_sum = 0;
		memset(input_array, 0, sizeof(input_array));
		memset(expected_array, 0, sizeof(expected_array));
		memset(usable, 0, sizeof(usable));
		memset(paratemp, -1, sizeof(paratemp));
		reset();
		reset_cmd();
	}
} 
