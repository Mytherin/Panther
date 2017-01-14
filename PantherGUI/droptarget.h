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
	HRESULT DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	HRESULT DragLeave(void);
	HRESULT Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

	HRESULT QueryInterface(REFIID iid, void ** ppvObject);
	ULONG AddRef(void);
	ULONG Release(void);
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
	PGDropSource(PGDropCallback callback, void* data) : callback(callback), data(data), m_lRefCount(1) {}

	HRESULT GiveFeedback(DWORD dwEffect);
	HRESULT QueryContinueDrag(BOOL  fEscapePressed, DWORD grfKeyState);

	HRESULT QueryInterface(REFIID iid, void ** ppvObject);
	ULONG AddRef(void);
	ULONG Release(void);
private:
	PGDropCallback callback;
	void* data;

	LONG	m_lRefCount;
};

class PGDataObject : public IDataObject {
public:
	PGDataObject(PGWindowHandle handle, PGDropCallback callback, void* data);
	~PGDataObject();

	HRESULT QueryGetData(FORMATETC *pformatetc);
	HRESULT GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);

	HRESULT QueryInterface(REFIID iid, void ** ppvObject);
	ULONG AddRef(void);
	ULONG Release(void);

	HRESULT DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) { return E_NOTIMPL; }
	HRESULT DUnadvise(DWORD dwConnection) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT EnumDAdvise(IEnumSTATDATA **ppenumAdvise) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) { return E_NOTIMPL; }
	HRESULT GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut) {
		pformatetcOut->ptd = NULL;
		return E_NOTIMPL;
	}
	HRESULT GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) {
		return DATA_E_FORMATETC;;
	}
	HRESULT SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) {
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
