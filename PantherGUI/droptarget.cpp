
#include "droptarget.h"
#include "main.h"
#include "windows_structs.h"

#include <shlobj.h>

PGDropTarget::PGDropTarget(PGWindowHandle handle) : handle(handle), m_hWnd(handle->hwnd), m_lRefCount(1) {
}

PGDropTarget::~PGDropTarget() {

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
	}
	return S_OK;
}

HRESULT PGDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
	if (m_fAllowDrop) {
		if (fmt == PG_CLIPBOARD_FORMAT) {
			PGPoint mouse = GetMousePosition(handle);
			handle->manager->DragDrop(PGDragDropTabs, mouse.x, mouse.y, data);
		}
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

HRESULT PGDropTarget::DragLeave(void) {
	if (m_fAllowDrop) {
		if (fmt == PG_CLIPBOARD_FORMAT) {
			handle->manager->ClearDragDrop(PGDragDropTabs);
		}
	}
	return S_OK;
}

HRESULT PGDropTarget::Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
	if (m_fAllowDrop) {
		// construct a FORMATETC object
		FORMATETC fmtetc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgmed;

		// See if the dataobject contains any TEXT stored as a HGLOBAL
		if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
			// Yippie! the data is there, so go get it!
			if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK) {
				// we asked for the data as a HGLOBAL, so access it appropriately
				PVOID data = GlobalLock(stgmed.hGlobal);
				DROPFILES* files = (DROPFILES*)data;
				char* filename_ptr = ((char*)files) + files->pFiles;
				std::string filename = UCS2toUTF8((PWSTR)filename_ptr);
				GlobalUnlock(stgmed.hGlobal);
				// release the data using the COM API
				ReleaseStgMedium(&stgmed);
				DropFile(handle, filename);
			}
			return S_OK;
		}
		fmtetc.cfFormat = PG_CLIPBOARD_FORMAT;
		if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
			PGPoint mouse = GetMousePosition(handle);
			handle->manager->PerformDragDrop(PGDragDropTabs, mouse.x, mouse.y, data);
		}

		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;
}

DWORD PGDropTarget::DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed) {
	DWORD dwEffect = 0;

	if (fmt == PG_CLIPBOARD_FORMAT) {
		dwEffect = dwAllowed & DROPEFFECT_MOVE;
		return dwEffect;
	}

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
	FORMATETC fmtetc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
		this->fmt = CF_HDROP;
		return true;
	}

	fmtetc.cfFormat = PG_CLIPBOARD_FORMAT;
	if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
		this->fmt = PG_CLIPBOARD_FORMAT;
		STGMEDIUM medium;
		pDataObject->GetData(&fmtetc, &medium);
		this->data = (void*)medium.pstg;
		return true;
	}

	this->fmt = -1;

	return false;
}

void RegisterDropWindow(PGWindowHandle handle, IDropTarget **ppDropTarget) {
	PGDropTarget *pDropTarget = new PGDropTarget(handle);

	// acquire a strong lock
	CoLockObjectExternal(pDropTarget, TRUE, FALSE);

	// tell OLE that the window is a drop target
	RegisterDragDrop(handle->hwnd, pDropTarget);

	*ppDropTarget = pDropTarget;
}

void UnregisterDropWindow(PGWindowHandle handle, IDropTarget *pDropTarget) {
	// remove drag+drop
	RevokeDragDrop(handle->hwnd);

	// remove the strong lock
	CoLockObjectExternal(pDropTarget, FALSE, TRUE);

	// release our own reference
	pDropTarget->Release();
}

HRESULT PGDropSource::GiveFeedback(DWORD dwEffect) {
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT PGDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {
	if (drop_cancelled) {
		return DRAGDROP_S_CANCEL;
	}
	if (fEscapePressed) {
		return DRAGDROP_S_CANCEL;
	}
	if ((grfKeyState & MK_LBUTTON) == 0) {
		return DRAGDROP_S_DROP;
	}
	return S_OK;
}

HRESULT PGDropSource::QueryInterface(REFIID iid, void ** ppvObject) {
	if (iid == IID_IDropSource || iid == IID_IUnknown) {
		AddRef();
		*ppvObject = this;
		return S_OK;
	} else {
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

ULONG PGDropSource::AddRef(void) {
	return InterlockedIncrement(&m_lRefCount);
}

ULONG PGDropSource::Release(void) {
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0) {
		delete this;
		return 0;
	} else {
		return count;
	}
}

PGDataObject::PGDataObject(PGWindowHandle handle, PGDropCallback callback, void* data) : 
	handle(handle), callback(callback), data(data), m_lRefCount(0) {
}

PGDataObject::~PGDataObject() {
}


HRESULT PGDataObject::QueryGetData(FORMATETC *pformatetc) {
	if (pformatetc->cfFormat == PG_CLIPBOARD_FORMAT)
		return S_OK;
	return DV_E_FORMATETC;
}

HRESULT PGDataObject::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium) {
	if (pformatetcIn->cfFormat == PG_CLIPBOARD_FORMAT) {
		pmedium->pstg = (IStorage*)data;
		return S_OK;
	}
	return DV_E_FORMATETC;
}

HRESULT PGDataObject::QueryInterface(REFIID iid, void ** ppvObject) {
	if (iid == IID_IDataObject || iid == IID_IUnknown) {
		AddRef();
		*ppvObject = this;
		return S_OK;
	} else {
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

ULONG PGDataObject::AddRef(void) {
	return InterlockedIncrement(&m_lRefCount);
}

ULONG PGDataObject::Release(void) {
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0) {
		POINT point;
		if (GetCursorPos(&point)) {
			callback(PGPoint(point.x, point.y), data);
		}
		delete this;
		return 0;
	} else {
		return count;
	}
}

PGWindowHandle active_dragging_window = nullptr;

void PGPerformDragDrop(PGWindowHandle window) {
	assert(!window->dragging);
	window->dragging = true;

	active_dragging_window = window;

	PGDataObject* object = new PGDataObject(window, window->drag_drop_data.callback, window->drag_drop_data.data);
	PGDropSource* source = new PGDropSource(window->drag_drop_data.callback, window->drag_drop_data.data);
	DWORD result;

	window->source = source;
	HRESULT hresult = DoDragDrop(object, source, DROPEFFECT_MOVE | DROPEFFECT_COPY, &result);
	window->source = nullptr;

	window->dragging = false;
}


void PGStartDragDrop(PGWindowHandle window, PGBitmapHandle image, PGDropCallback callback, void* data, size_t data_length) {
	window->pending_drag_drop = true;
	window->drag_drop_data.image = image;
	window->drag_drop_data.callback = callback;
	window->drag_drop_data.data = data;
	window->drag_drop_data.data_length = data_length;
}

void PGCancelDragDrop(PGWindowHandle window) {
	assert(active_dragging_window);
	assert(active_dragging_window->dragging);
	if (active_dragging_window->dragging) {
		active_dragging_window->source->CancelDrop();
	}
}