#pragma once

#include <windows.h>
#include <objidl.h>

#include "windowfunctions.h"

#define PG_CLIPBOARD_FORMAT CF_PRIVATEFIRST

class PGDropTarget : public IDropTarget {
public:
	// Constructor
	PGDropTarget(PGWindowHandle handle);
	~PGDropTarget();

	// IDropTarget implementation
	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	HRESULT STDMETHODCALLTYPE DragLeave(void);
	HRESULT STDMETHODCALLTYPE Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
private:
	PGWindowHandle handle;
	// internal helper function
	DWORD DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed);
	bool  QueryDataObject(IDataObject *pDataObject);

	// Private member variables
	LONG	m_lRefCount;
	HWND   m_hWnd;
	bool   m_fAllowDrop;

	int fmt;
	void* data;

	IDataObject *m_pDataObject;
};


class PGDropSource : public IDropSource {
public:
	PGDropSource(PGDropCallback callback, void* data) : callback(callback), data(data), drop_cancelled(false), m_lRefCount(1){}

	HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);
	HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL  fEscapePressed, DWORD grfKeyState);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	void CancelDrop() { drop_cancelled = true; }
private:
	PGDropCallback callback;
	void* data;
	bool drop_cancelled = false;

	LONG	m_lRefCount;
};

class PGDataObject : public IDataObject {
public:
	PGDataObject(PGWindowHandle handle, PGDropCallback callback, void* data);
	~PGDataObject();

	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *pformatetc);
	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **ppenumAdvise) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut) {
		pformatetcOut->ptd = NULL;
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) {
		return DATA_E_FORMATETC;;
	}
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) {
		return E_NOTIMPL;
	}

private:
	PGWindowHandle handle;
	PGDropCallback callback;
	void* data;
	LONG	m_lRefCount;
};

void RegisterDropWindow(PGWindowHandle handle, IDropTarget **ppDropTarget);
void UnregisterDropWindow(PGWindowHandle handle, IDropTarget *pDropTarget);
void PGPerformDragDrop(PGWindowHandle window);
