
#include "droptarget.h"

//
//	Position the edit control's caret under the mouse
//
/*void PositionCursor(HWND hwndEdit, POINTL pt) {
	DWORD curpos;

	// get the character position of mouse
	ScreenToClient(hwndEdit, (POINT *)&pt);
	curpos = SendMessage(hwndEdit, EM_CHARFROMPOS, 0, MAKELPARAM(pt.x, pt.y));

	// set cursor position
	SendMessage(hwndEdit, EM_SETSEL, LOWORD(curpos), LOWORD(curpos));
}*/


PGDropTarget::PGDropTarget(HWND hwnd) : m_hWnd(hwnd) {

}

PGDropTarget::~PGDropTarget() {

}

void DropData(HWND hwnd, IDataObject *pDataObject) {
	// construct a FORMATETC object
	FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed;

	// FIXME
	// See if the dataobject contains any TEXT stored as a HGLOBAL
	if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
		// Yippie! the data is there, so go get it!
		if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK) {
			// we asked for the data as a HGLOBAL, so access it appropriately
			PVOID data = GlobalLock(stgmed.hGlobal);

			SetWindowText(hwnd, (char *)data);

			GlobalUnlock(stgmed.hGlobal);

			// release the data using the COM API
			ReleaseStgMedium(&stgmed);
		}
	}
}

HRESULT PGDropTarget::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
	// does the dataobject contain data we want?
	m_fAllowDrop = QueryDataObject(pDataObject);

	if (m_fAllowDrop) {
		// get the dropeffect based on keyboard state
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);

		SetFocus(m_hWnd);
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}    return S_OK;
}

HRESULT PGDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
	if (m_fAllowDrop) {
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

HRESULT PGDropTarget::DragLeave(void) {
	return S_OK;
}

HRESULT PGDropTarget::Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
	if (m_fAllowDrop) {
		DropData(m_hWnd, pDataObject);
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;
}

DWORD PGDropTarget::DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed) {
	DWORD dwEffect = 0;

	// 1. check "pt" -> do we allow a drop at the specified coordinates?
	// 2. work out that the drop-effect should be based on grfKeyState	
	if (grfKeyState & MK_CONTROL) {
		dwEffect = dwAllowed & DROPEFFECT_COPY;
	} else if (grfKeyState & MK_SHIFT) {
		dwEffect = dwAllowed & DROPEFFECT_MOVE;
	}

	// 3. no key-modifiers were specified (or drop effect not allowed), so
	//    base the effect on those allowed by the dropsource
	if (dwEffect == 0) {
		if (dwAllowed & DROPEFFECT_COPY) dwEffect = DROPEFFECT_COPY;
		if (dwAllowed & DROPEFFECT_MOVE) dwEffect = DROPEFFECT_MOVE;
	}

	return dwEffect;
}

HRESULT PGDropTarget::QueryInterface(REFIID iid, void ** ppvObject) {
	if (iid == IID_IDropTarget || iid == IID_IUnknown) {
		AddRef();
		*ppvObject = this;
		return S_OK;
	} else {
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

ULONG PGDropTarget::AddRef(void) {
	return InterlockedIncrement(&m_lRefCount);
}

ULONG PGDropTarget::Release(void) {
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0) {
		delete this;
		return 0;
	} else {
		return count;
	}
}

bool PGDropTarget::QueryDataObject(IDataObject *pDataObject) {
	return true;
}

void RegisterDropWindow(HWND hwnd, IDropTarget **ppDropTarget) {
	PGDropTarget *pDropTarget = new PGDropTarget(hwnd);

	// acquire a strong lock
	CoLockObjectExternal(pDropTarget, TRUE, FALSE);

	// tell OLE that the window is a drop target
	RegisterDragDrop(hwnd, pDropTarget);

	*ppDropTarget = pDropTarget;
}

void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget) {
	// remove drag+drop
	RevokeDragDrop(hwnd);

	// remove the strong lock
	CoLockObjectExternal(pDropTarget, FALSE, TRUE);

	// release our own reference
	pDropTarget->Release();
}
