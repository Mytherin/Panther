#pragma once

#include <Windows.h>

class PGDropTarget : public IDropTarget {
public:
	// Constructor
	PGDropTarget(HWND hwnd);
	~PGDropTarget();

	// IDropTarget implementation
	HRESULT DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	HRESULT DragLeave(void);
	HRESULT Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

	HRESULT QueryInterface (REFIID iid, void ** ppvObject);
	ULONG AddRef (void);
	ULONG Release (void);
private:
	// internal helper function
	DWORD DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed);
	bool  QueryDataObject(IDataObject *pDataObject);

	// Private member variables
	LONG	m_lRefCount;
	HWND   m_hWnd;
	bool   m_fAllowDrop;

	IDataObject *m_pDataObject;
};

void RegisterDropWindow(HWND hwnd, IDropTarget **ppDropTarget);
void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget);
