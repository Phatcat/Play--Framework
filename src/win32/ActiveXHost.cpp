#include <assert.h>
#include "win32/ActiveXHost.h"

using namespace Framework;
using namespace Framework::Win32;

CActiveXHost::CActiveXHost()
{
	m_hWnd = NULL;
}

CActiveXHost::CActiveXHost(HWND parentWnd, const RECT& rect, const CLSID& clsid)
{
	Create(WS_EX_CONTROLPARENT, CDefaultWndClass::GetName(), _T("ActiveXHost"), WS_VISIBLE | WS_CHILD, rect, parentWnd, NULL);
	SetClassPtr();

	m_clientSite = CComPtr<CClientSite>(new CClientSite(m_hWnd));

	HRESULT result = StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_CREATE, NULL, &m_storage);
	assert(SUCCEEDED(result));

	result = OleCreate(clsid, IID_IOleObject, OLERENDER_DRAW, NULL, m_clientSite, m_storage, reinterpret_cast<void**>(&m_oleObject));
	assert(SUCCEEDED(result));

	result = OleSetContainedObject(m_oleObject, TRUE);
	assert(SUCCEEDED(result));

	result = m_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, 0, m_clientSite, 0, m_hWnd, &rect);
	assert(SUCCEEDED(result));
}

CActiveXHost::~CActiveXHost()
{
	Reset();
}

void CActiveXHost::Reset()
{
	if(!m_oleObject.IsEmpty())
	{
		CComPtr<IOleInPlaceObject> inPlaceObject;
		HRESULT result = m_oleObject->QueryInterface(IID_IOleInPlaceObject, reinterpret_cast<void**>(&inPlaceObject));
		if(SUCCEEDED(result))
		{
			inPlaceObject->UIDeactivate();
			inPlaceObject->InPlaceDeactivate();
		}

		m_oleObject->Close(OLECLOSE_NOSAVE);
		m_oleObject->SetClientSite(NULL);
	}

	m_oleObject.Reset();
	m_storage.Reset();
	m_clientSite.Reset();
	CWindow::Reset();
}

void CActiveXHost::MoveFrom(CActiveXHost&& rhs)
{
	CWindow::MoveFrom(std::move(rhs));
	std::swap(m_oleObject, rhs.m_oleObject);
	std::swap(m_storage, rhs.m_storage);
	std::swap(m_clientSite, rhs.m_clientSite);
}

long CActiveXHost::OnSize(unsigned int type, unsigned int width, unsigned int height)
{
	if(!m_oleObject.IsEmpty())
	{
		CComPtr<IOleInPlaceObject> inPlaceObject;
		HRESULT result = m_oleObject->QueryInterface(IID_IOleInPlaceObject, reinterpret_cast<void**>(&inPlaceObject));
		if(SUCCEEDED(result))
		{
			Framework::Win32::CRect newRect(0, 0, width, height);
			inPlaceObject->SetObjectRects(newRect, newRect);
		}
	}

	return FALSE;
}

CActiveXHost::CClientSite::CClientSite(HWND window)
: m_refCount(1)
, m_window(window)
{

}

CActiveXHost::CClientSite::~CClientSite()
{

}

ULONG CActiveXHost::CClientSite::AddRef()
{
	InterlockedIncrement(&m_refCount);
	return m_refCount;
}

ULONG CActiveXHost::CClientSite::Release()
{
	InterlockedDecrement(&m_refCount);
	if(m_refCount == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return m_refCount;
	}
}

HRESULT CActiveXHost::CClientSite::QueryInterface(const IID& iid, void** intrf)
{
	(*intrf) = NULL;
	if(iid == IID_IOleInPlaceSite)
	{
		(*intrf) = static_cast<IOleInPlaceSite*>(this);
	}
	else if(iid == IID_IDispatch)
	{
//		(*intrf) = static_cast<IDispatch*>(this);
	}
	else if(iid == IID_IAdviseSink)
	{
//		(*intrf) = static_cast<IAdviseSink*>(this);
	}

	if(*intrf)
	{
		AddRef();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

HRESULT CActiveXHost::CClientSite::SaveObject()
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetMoniker(DWORD, DWORD, IMoniker**)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetContainer(IOleContainer** container)
{
	(*container) = NULL;
	return E_FAIL;
}

HRESULT CActiveXHost::CClientSite::ShowObject()
{
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::OnShowWindow(BOOL)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::RequestNewObjectLayout()
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetWindow(HWND* hwndPtr)
{
	(*hwndPtr) = m_window;
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::ContextSensitiveHelp(BOOL)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::CanInPlaceActivate()
{
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::OnUIActivate()
{
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::OnInPlaceActivate()
{
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::OnUIDeactivate(BOOL)
{
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::OnInPlaceDeactivate()
{
	return S_OK;
}

HRESULT CActiveXHost::CClientSite::GetWindowContext(IOleInPlaceFrame** frame, IOleInPlaceUIWindow** doc, LPRECT posRect, LPRECT clipRect, LPOLEINPLACEFRAMEINFO frameInfo)
{
	(*frame) = this;
	AddRef();

	(*doc) = nullptr;

	::GetClientRect(m_window, posRect);
	::GetClientRect(m_window, clipRect);

	frameInfo->cb				= sizeof(OLEINPLACEFRAMEINFO);
	frameInfo->cAccelEntries	= 0;
	frameInfo->haccel			= NULL;
	frameInfo->fMDIApp			= FALSE;
	frameInfo->hwndFrame		= m_window;

	return S_OK;
}

HRESULT CActiveXHost::CClientSite::Scroll(SIZE)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::DiscardUndoState()
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::DeactivateAndUndo()
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::OnPosRectChange(LPCRECT)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetBorder(LPRECT)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::RequestBorderSpace(LPCBORDERWIDTHS)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::SetBorderSpace(LPCBORDERWIDTHS)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::SetActiveObject(IOleInPlaceActiveObject*, LPCOLESTR)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::SetMenu(HMENU, HOLEMENU, HWND)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::RemoveMenus(HMENU)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::EnableModeless(BOOL)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::TranslateAccelerator(LPMSG, WORD)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::SetStatusText(LPCOLESTR)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetTypeInfoCount(UINT*)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::GetIDsOfNames(const IID&, LPOLESTR*, UINT, LCID, DISPID*)
{
	return E_NOTIMPL;
}

HRESULT CActiveXHost::CClientSite::Invoke(DISPID, const IID&, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)
{
	return E_NOTIMPL;
}

void CActiveXHost::CClientSite::OnDataChange(FORMATETC*, STGMEDIUM*)
{

}

void CActiveXHost::CClientSite::OnViewChange(DWORD, LONG)
{

}

void CActiveXHost::CClientSite::OnRename(IMoniker*)
{

}

void CActiveXHost::CClientSite::OnSave()
{

}

void CActiveXHost::CClientSite::OnClose()
{

}