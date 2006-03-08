#include "GUI_Window.h"
#include "PlatformUtils.h"

static set<GUI_Window *>	sWindows;

GUI_Window::GUI_Window(const char * inTitle, int inBounds[4]) :
	XWinGL(inTitle, inBounds[0], inBounds[1], inBounds[2]-inBounds[0],inBounds[3]-inBounds[1], sWindows.empty() ? NULL : *sWindows.begin())
{
	mKeyFocus = this;
	sWindows.insert(this);
	mBounds[0] = 0;
	mBounds[1] = 0;
	mBounds[2] = inBounds[2] - inBounds[0];
	mBounds[3] = inBounds[3] - inBounds[1];
	mMouseFocusPane = NULL;
	mVisible = true;
	mClearColorRGBA[0] = 1.0;
	mClearColorRGBA[1] = 1.0;
	mClearColorRGBA[2] = 1.0;
	mClearColorRGBA[3] = 1.0;
	mClearDepth = false;;
	mClearColor = true;
	mDesc = inTitle;
	mState.Init();
}

void	GUI_Window::SetClearSpecs(bool inDoClearColor, bool inDoClearDepth, float inClearColor[4])
{
	mClearColorRGBA[0] = inClearColor[0];
	mClearColorRGBA[1] = inClearColor[1];
	mClearColorRGBA[2] = inClearColor[2];
	mClearColorRGBA[3] = inClearColor[3];
	mClearDepth = inDoClearDepth;
	mClearColor = inDoClearColor;
}



GUI_Window::~GUI_Window()
{
	sWindows.erase(this);
}

void			GUI_Window::ClickDown(int inX, int inY, int inButton)
{
	int	w, h;
	XWinGL::GetBounds(&w, &h);

	mMouseFocusPane = InternalMouseDown(inX, h-inY, inButton);
}

void			GUI_Window::ClickUp(int inX, int inY, int inButton)
{
	int	w, h;
	XWinGL::GetBounds(&w, &h);

	if (mMouseFocusPane)
		mMouseFocusPane->MouseUp(inX, h-inY, inButton);
	mMouseFocusPane = NULL;
}

void			GUI_Window::ClickDrag(int inX, int inY, int inButton)
{
	int	w, h;
	XWinGL::GetBounds(&w, &h);

	if (mMouseFocusPane)
		mMouseFocusPane->MouseDrag(inX, h-inY, inButton);
}

void			GUI_Window::MouseWheel(int inX, int inY, int inDelta, int inAxis)
{
	int	w, h;
	XWinGL::GetBounds(&w, &h);

	InternalMouseWheel(inX, h-inY, inDelta, inAxis);
}
	
void			GUI_Window::GLReshaped(int inWidth, int inHeight)
{
//	int bounds[4] = { 0, 0, inWidth, inHeight };
//	SetBounds(bounds);

	int oldBounds[4] = { mBounds[0], mBounds[1], mBounds[2], mBounds[3] }; 

	mBounds[0] = 0;
	mBounds[1] = 0;
	mBounds[2] = inWidth;
	mBounds[3] = inHeight;
	
	if (oldBounds[0] != mBounds[0] ||
		oldBounds[1] != mBounds[1] ||
		oldBounds[2] != mBounds[2] ||
		oldBounds[3] != mBounds[3])
	{
		for (vector<GUI_Pane *>::iterator c = mChildren.begin(); c != mChildren.end(); ++c)
			(*c)->ParentResized(oldBounds, mBounds);
		Refresh();
	}	
}

void			GUI_Window::GLDraw(void)
{
	SetGLContext();
	
	int	w, h;
	XWinGL::GetBounds(&w, &h);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (mClearColor)
		glClearColor(mClearColorRGBA[0],mClearColorRGBA[1],mClearColorRGBA[2],mClearColorRGBA[3]);
	glClear((mClearColor ? GL_COLOR_BUFFER_BIT : 0) + (mClearDepth ? GL_DEPTH_BUFFER_BIT : 0));
	mState.Reset();
	InternalDraw(&mState);
}

void		GUI_Window::Refresh(void)
{
	ForceRefresh();
}

void	GUI_Window::Show(void)
{
	mVisible = true;
	SetVisible(true);
}

void GUI_Window::Hide(void)
{
	mVisible = false;
	SetVisible(false);
}

void	GUI_Window::SetBounds(int inBounds[4])
{
	int oldBounds[4] = { mBounds[0], mBounds[1], mBounds[2], mBounds[3] };

	XWinGL::MoveTo(inBounds[0], inBounds[1]);
	XWinGL::Resize(inBounds[2]-inBounds[0], inBounds[3]-inBounds[1]);
}

void		GUI_Window::SetBounds(int x1, int y1, int x2, int y2) 
{
	int b[4] = { x1, y1, x2, y2 };
	SetBounds(b);
}

void		GUI_Window::SetDescriptor(const string& inDesc)
{
	mDesc = inDesc;
	XWinGL::SetTitle(inDesc.c_str());
}


int			GUI_Window::InternalSetFocus(GUI_Pane * who)
{
	mKeyFocus = who;
	return 1;
}

GUI_Pane *	GUI_Window::GetFocus(void)
{
	return mKeyFocus;
}

int			GUI_Window::AcceptTakeFocus(void)
{
	return 1;		// Window is the focuser of last resort -- like the federal reserve.
}

#if APL

const char	gui_Key_Map [256] = {
/*			00					01					02					03					04					05					06					07*/
/* 00 */	GUI_VK_A,			GUI_VK_S,			GUI_VK_D,			GUI_VK_F,			GUI_VK_H,			GUI_VK_G,			GUI_VK_Z,			GUI_VK_X,
/* 08 */	GUI_VK_C,			GUI_VK_V,			0,					GUI_VK_B,			GUI_VK_Q,			GUI_VK_W,			GUI_VK_E,			GUI_VK_R,
/* 10 */	GUI_VK_Y,			GUI_VK_T,			GUI_VK_1,			GUI_VK_2,			GUI_VK_3,			GUI_VK_4,			GUI_VK_6,			GUI_VK_5,
/* 18 */	GUI_VK_EQUAL,		GUI_VK_9,			GUI_VK_7,			GUI_VK_MINUS,		GUI_VK_8,			GUI_VK_0,			GUI_VK_RBRACE,		GUI_VK_O,
/* 20 */	GUI_VK_U,			GUI_VK_LBRACE,		GUI_VK_I,			GUI_VK_P,			GUI_VK_RETURN,		GUI_VK_L,			GUI_VK_J,			GUI_VK_QUOTE,
/* 28 */	GUI_VK_K,			GUI_VK_SEMICOLON,	GUI_VK_BACKSLASH,	GUI_VK_COMMA,		GUI_VK_SLASH,		GUI_VK_N,			GUI_VK_M,			GUI_VK_PERIOD,
/* 30 */	GUI_VK_TAB,		GUI_VK_SPACE,		GUI_VK_BACKQUOTE,	GUI_VK_DELETE,		GUI_VK_ENTER,		GUI_VK_ESCAPE,		0,					0,
/* 38 */	0,					0,					0,					0,					0,					0,					0,					0,
/* 40 */	0,					GUI_VK_DECIMAL,	0,					GUI_VK_MULTIPLY,	0,					GUI_VK_ADD,		0,					GUI_VK_CLEAR,
/* 48 */	0,					0,					0,					GUI_VK_DIVIDE,		GUI_VK_NUMPAD_ENT,	0,					GUI_VK_SUBTRACT,	0,
/* 50 */	0,					GUI_VK_NUMPAD_EQ,	GUI_VK_NUMPAD0,	GUI_VK_NUMPAD1,	GUI_VK_NUMPAD2,	GUI_VK_NUMPAD3,	GUI_VK_NUMPAD4,	GUI_VK_NUMPAD5,
/* 58 */	GUI_VK_NUMPAD6,	GUI_VK_NUMPAD7,	0,					GUI_VK_NUMPAD8,	GUI_VK_NUMPAD9,	0,					0,					0,
/* 60 */	GUI_VK_F5,			GUI_VK_F6,			GUI_VK_F7,			GUI_VK_F3,			GUI_VK_F8,			GUI_VK_F9,			0,					GUI_VK_F11	,	
/* 68 */	0,					0,					0,					0,					0,					GUI_VK_F10,		0,					GUI_VK_F12,
/* 70 */	0,					0,					0,					GUI_VK_HOME,		GUI_VK_PRIOR,		0,					GUI_VK_F4,			GUI_VK_END,		
/* 78 */	GUI_VK_F2,			GUI_VK_NEXT,		GUI_VK_F1,			GUI_VK_LEFT,		GUI_VK_RIGHT,		GUI_VK_DOWN,		GUI_VK_UP,			0
};

#else

const char	gui_Key_Map [256] = {
/*			00					01					02					03					04					05					06					07*/
/* 00 */	0,					0,					0,					0,					0,					0,					0,					0,
/* 08 */	GUI_VK_BACK,		GUI_VK_TAB,		0,					0,					GUI_VK_CLEAR,		GUI_VK_RETURN,		0,					0,
/* 10 */	0,					0,					0,					0,					0,					0,					0,					0,
/* 18 */	0,					0,					0,					GUI_VK_ESCAPE,		0,					0,					0,					0,
/* 20 */	GUI_VK_SPACE,		GUI_VK_PRIOR,		GUI_VK_NEXT,		GUI_VK_END,		GUI_VK_HOME,		GUI_VK_LEFT,		GUI_VK_UP,			GUI_VK_RIGHT,
/* 28 */	GUI_VK_DOWN,		GUI_VK_SELECT,		GUI_VK_PRINT,		GUI_VK_EXECUTE,	GUI_VK_SNAPSHOT,	GUI_VK_INSERT,		GUI_VK_DELETE,		GUI_VK_HELP,
/* 30 */	GUI_VK_0,			GUI_VK_1,			GUI_VK_2,			GUI_VK_3,			GUI_VK_4,			GUI_VK_5,			GUI_VK_6,			GUI_VK_7,
/* 38 */	GUI_VK_8,			GUI_VK_9,			0,					0,					0,					0,					0,					0,
/* 40 */	0,					GUI_VK_A,			GUI_VK_B,			GUI_VK_C,			GUI_VK_D,			GUI_VK_E,			GUI_VK_F,			GUI_VK_G,
/* 48 */	GUI_VK_H,			GUI_VK_I,			GUI_VK_J,			GUI_VK_K,			GUI_VK_L,			GUI_VK_M,			GUI_VK_N,			GUI_VK_O,
/* 50 */	GUI_VK_P,			GUI_VK_Q,			GUI_VK_R,			GUI_VK_S,			GUI_VK_T,			GUI_VK_U,			GUI_VK_V,			GUI_VK_W,
/* 58 */	GUI_VK_X,			GUI_VK_Y,			GUI_VK_Z,			0,					0,					0,					0,					0,
/* 60 */	GUI_VK_NUMPAD0,	GUI_VK_NUMPAD1,	GUI_VK_NUMPAD2,	GUI_VK_NUMPAD3,	GUI_VK_NUMPAD4,	GUI_VK_NUMPAD5,	GUI_VK_NUMPAD6,	GUI_VK_NUMPAD7,
/* 68 */	GUI_VK_NUMPAD8,	GUI_VK_NUMPAD9,	GUI_VK_MULTIPLY,	GUI_VK_ADD,		0,					GUI_VK_SUBTRACT,	GUI_VK_DECIMAL,	GUI_VK_DIVIDE,
/* 70 */	GUI_VK_F1,			GUI_VK_F2,			GUI_VK_F3,			GUI_VK_F4,			GUI_VK_F5,			GUI_VK_F6,			GUI_VK_F7,			GUI_VK_F8,
/* 78 */	GUI_VK_F9,			GUI_VK_F10,		GUI_VK_F11,		GUI_VK_F12,		GUI_VK_F13,		GUI_VK_F14,		GUI_VK_F15,		GUI_VK_F16,
/* 80 */	GUI_VK_F17,		GUI_VK_F18,		GUI_VK_F19,		GUI_VK_F20,		GUI_VK_F21,		GUI_VK_F22,		GUI_VK_F23,		GUI_VK_F24,
/* 88 */	0,					0,					0,					0,					0,					0,					0,					0,
/* 90 */	0,					0,					0,					0,					0,					0,					0,					0,
/* 88 */	0,					0,					0,					0,					0,					0,					0,					0,
/* A0 */	0,					0,					0,					0,					0,					0,					0,					0,
/* A8 */	0,					0,					0,					0,					0,					0,					0,					0,
/* B0 */	0,					0,					0,					0,					0,					0,					0,					0,
/* B8 */	0,					0,					GUI_VK_SEMICOLON,	GUI_VK_EQUAL,		GUI_VK_COMMA,		GUI_VK_MINUS,		GUI_VK_PERIOD,		GUI_VK_SLASH,
/* C0 */	GUI_VK_BACKQUOTE,	0,					0,					0,					0,					0,					0,					0,
/* C8 */	0,					0,					0,					0,					0,					0,					0,					0,
/* D0 */	0,					0,					0,					0,					0,					0,					0,					0,
/* D8 */	0,					0,					0,					GUI_VK_LBRACE,		GUI_VK_BACKSLASH,	GUI_VK_RBRACE,		GUI_VK_QUOTE,		GUI_VK_BACKQUOTE,
/* E0 */	0,					0,					0,					0,					0,					0,					0,					0,
/* E8 */	0,					0,					0,					0,					0,					0,					0,					0,
/* F0 */	0,					0,					0,					0,					0,					0,					0,					0,
/* F8 */	0,					0,					0,					0,					0,					0,					0,					0
};


#endif


int			GUI_Window::KeyPressed(char inKey, long inMsg, long inParam1, long inParam2)
{
	GUI_KeyFlags		flags = 0;
#if APL
	char		charCode = 0;
	char		virtualCode = 0;
#elif IBM
	unsigned char	charCode = 0;
	unsigned char	virtualCode = 0;

	HKL hKL;
	unsigned int vKey, RetCode, ScanCode;
	unsigned short Char = 0;
	BYTE KeyState[256];
	long shiftKey, controlKey, optionKey, keyDown, keyUp, charCodeMask,	keyCodeMask;
	long ExtKeyMask, ShiftControlMask, scrollLockKey, capsLockKey, numLockKey;
	int ExtendedKey;
#endif

#if APL

	charCode = inParam1 & charCodeMask;
	virtualCode = (inParam1 & keyCodeMask) >> 8L;
	if (inParam2 & shiftKey)
		flags |= gui_ShiftFlag;
	if (inParam2 & cmdKey)
		flags |= gui_ControlFlag;
	if (inParam2 & optionKey)
		flags |= gui_OptionAltFlag;
	if (inMsg == keyDown)
		flags |= gui_DownFlag;
	if (inMsg == keyUp)
		flags |= gui_UpFlag;
		
	// NOTE: the GUI_KEY ASCII defines are all mac-compatible.

	// Finally, control and option keys are not available as ASCII because
	// the ASCII codes are whacko.
	if (inParam2 & (controlKey + optionKey + cmdKey))
		charCode = 0;

#elif IBM
	numLockKey = 0x01450000;
	capsLockKey =0x003a0000;
	scrollLockKey = 0x00460000;
	shiftKey = 0x002a0000;
	controlKey = 0x001d0000;
	optionKey = 0x00380000;
	keyDown = WM_KEYDOWN;
	keyUp = WM_KEYUP;
	charCodeMask = 0xff;
	keyCodeMask = 0xff;
	ShiftControlMask = 0x00ff0000;
	ExtKeyMask = 0x01ff0000;

	if (((inParam2 & ExtKeyMask) == numLockKey) || ((inParam2 & ShiftControlMask) == capsLockKey) || ((inParam2 & ShiftControlMask) == scrollLockKey))
		return 1;

	if (((inParam2 & ShiftControlMask) == shiftKey) || (inParam1 == VK_SHIFT))
	{
		if (inMsg == keyDown)
			ShiftToggle = 1;
		else
			ShiftToggle = 0;

		return 1;
	}

	if ((inParam2 & ShiftControlMask) == controlKey)
	{
		if (inMsg == keyDown)
			ControlToggle = 1;
		else
			ControlToggle = 0;

		return 1;
	}

	/// SB
	if ((inParam2 & ShiftControlMask) == optionKey)
	{
		if (inMsg == keyDown)
			OptionKeyToggle = 1;
		else
			OptionKeyToggle = 0;

		return 1;
	}

	hKL = GetKeyboardLayout(NULL); 
	ScanCode = ((inParam2 >> 16) & 0xff);
	ExtendedKey =  ((inParam2 >> 24) & 0x01);
	vKey = MapVirtualKeyEx(ScanCode, 1, hKL); 
	RetCode = GetKeyboardState((unsigned char*)&KeyState);
	RetCode = ToAsciiEx(vKey, ScanCode, (unsigned char*)&KeyState, &Char, 0, hKL); 

	if (RetCode != 0)
	{
		charCode = Char;
		if (ExtendedKey == 0)
			virtualCode = vKey;
		else
			virtualCode = inParam1 & keyCodeMask;
	}
	else
	{
		charCode = inParam1 & charCodeMask;
		virtualCode = inParam1 & keyCodeMask;
	}

	if (ShiftToggle)
		flags |= gui_ShiftFlag;
	if (ControlToggle)
		flags |= gui_ControlFlag;
///	SB if ((inParam2 & ShiftControlMask) == optionKey)
	if (OptionKeyToggle)
		flags |= gui_OptionAltFlag;
	if (inMsg == keyDown)
		flags |= gui_DownFlag;
	if (inMsg == keyUp)
		flags |= gui_UpFlag;
		
	// NOTE: the GUI_KEY ASCII defines are all mac-compatible.
		
	// Finally, control and option keys are not available as ASCII because
	// the ASCII codes are whacko.
	if ( ((inParam2 & ShiftControlMask) == controlKey) || ((inParam2 & ShiftControlMask) == optionKey))
		charCode = 0;
				
#else
	#error "Must port XPLM to a new OS...key bindings come in in an OS native form."
#endif

	virtualCode = gui_Key_Map[virtualCode];

#if IBM
	switch (virtualCode)
	{
		case GUI_VK_RETURN:			charCode = GUI_KEY_RETURN;		break;
		case GUI_VK_ESCAPE:			charCode = GUI_KEY_ESCAPE;		break;
		case GUI_VK_TAB:			charCode = GUI_KEY_TAB;			break;
		case GUI_VK_BACK:			charCode = GUI_KEY_DELETE;		break;
		case GUI_VK_LEFT:			charCode = GUI_KEY_LEFT;		break;
		case GUI_VK_UP:				charCode = GUI_KEY_UP;			break;
		case GUI_VK_RIGHT:			charCode = GUI_KEY_RIGHT;		break;
		case GUI_VK_DOWN:			charCode = GUI_KEY_DOWN;		break;
		case GUI_VK_NUMPAD0:		charCode = GUI_KEY_0;			break;
		case GUI_VK_NUMPAD1:		charCode = GUI_KEY_1;			break;
		case GUI_VK_NUMPAD2:		charCode = GUI_KEY_2;			break;
		case GUI_VK_NUMPAD3:		charCode = GUI_KEY_3;			break;
		case GUI_VK_NUMPAD4:		charCode = GUI_KEY_4;			break;
		case GUI_VK_NUMPAD5:		charCode = GUI_KEY_5;			break;
		case GUI_VK_NUMPAD6:		charCode = GUI_KEY_6;			break;
		case GUI_VK_NUMPAD7:		charCode = GUI_KEY_7;			break;
		case GUI_VK_NUMPAD8:		charCode = GUI_KEY_8;			break;
		case GUI_VK_NUMPAD9:		charCode = GUI_KEY_9;			break;
		case GUI_VK_DECIMAL:		charCode = GUI_KEY_DECIMAL;		break;
	}
#endif

	if ((flags == 0) && (charCode == 0) && (virtualCode == 0))
		return 1;	
		
	if (mKeyFocus)
	{
		for (GUI_Pane * p = mKeyFocus; p != NULL; p = p->mParent)
		{
			if (p->KeyPress(charCode, virtualCode, flags))
				return 1;
		}
	}
	// unconsumed char?

	return 0;		
}

