/*
 * Copyright © 2006 Sun Microsystems
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Sun Microsystems not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Sun Microsystems makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SUN MICROSYSTEMS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SUN MICROSYSTEMS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "compint.h"
#include "xace.h"

#define SERVER_COMPOSITE_MAJOR	0
#define SERVER_COMPOSITE_MINOR	4

static CARD8	CompositeReqCode;
static int CompositeClientPrivateKeyIndex;
static DevPrivateKey CompositeClientPrivateKey = &CompositeClientPrivateKeyIndex;
RESTYPE		CompositeClientWindowType;
RESTYPE		CompositeClientSubwindowsType;
RESTYPE		CompositeClientOverlayType;

typedef struct _CompositeClient {
    int	    major_version;
    int	    minor_version;
} CompositeClientRec, *CompositeClientPtr;

#define GetCompositeClient(pClient) ((CompositeClientPtr) \
    dixLookupPrivate(&(pClient)->devPrivates, CompositeClientPrivateKey))

static void
CompositeClientCallback (CallbackListPtr	*list,
		      pointer		closure,
		      pointer		data)
{
    NewClientInfoRec	*clientinfo = (NewClientInfoRec *) data;
    ClientPtr		pClient = clientinfo->client;
    CompositeClientPtr	pCompositeClient = GetCompositeClient (pClient);

    pCompositeClient->major_version = 0;
    pCompositeClient->minor_version = 0;
}

static int
FreeCompositeClientWindow (pointer value, XID ccwid)
{
    WindowPtr	pWin = value;

    compFreeClientWindow (pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientSubwindows (pointer value, XID ccwid)
{
    WindowPtr	pWin = value;

    compFreeClientSubwindows (pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientOverlay (pointer value, XID ccwid)
{
    CompOverlayClientPtr pOc = (CompOverlayClientPtr) value;

    compFreeOverlayClient (pOc);
    return Success;
}

static int
ProcCompositeQueryVersion (ClientPtr client)
{
    CompositeClientPtr pCompositeClient = GetCompositeClient (client);
    xCompositeQueryVersionReply rep;
    register int n;
    REQUEST(xCompositeQueryVersionReq);

    REQUEST_SIZE_MATCH(xCompositeQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    if (stuff->majorVersion < SERVER_COMPOSITE_MAJOR) {
	rep.majorVersion = stuff->majorVersion;
	rep.minorVersion = stuff->minorVersion;
    } else {
	rep.majorVersion = SERVER_COMPOSITE_MAJOR;
        rep.minorVersion = SERVER_COMPOSITE_MINOR;
    }
    pCompositeClient->major_version = rep.majorVersion;
    pCompositeClient->minor_version = rep.minorVersion;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.majorVersion, n);
	swapl(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xCompositeQueryVersionReply), (char *)&rep);
    return(client->noClientException);
}

static int
ProcCompositeRedirectWindow (ClientPtr client)
{
    WindowPtr	pWin;
    int rc;
    REQUEST(xCompositeRedirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);
    rc = dixLookupResourceByType((pointer *)&pWin, stuff->window, RT_WINDOW, client,
			   DixSetAttrAccess|DixManageAccess|DixBlendAccess);
    if (rc != Success)
    {
	client->errorValue = stuff->window;
	return (rc == BadValue) ? BadWindow : rc;
    }
    return compRedirectWindow (client, pWin, stuff->update);
}

static int
ProcCompositeRedirectSubwindows (ClientPtr client)
{
    WindowPtr	pWin;
    int rc;
    REQUEST(xCompositeRedirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);
    rc = dixLookupResourceByType((pointer *)&pWin, stuff->window, RT_WINDOW, client,
			   DixSetAttrAccess|DixManageAccess|DixBlendAccess);
    if (rc != Success)
    {
	client->errorValue = stuff->window;
	return (rc == BadValue) ? BadWindow : rc;
    }
    return compRedirectSubwindows (client, pWin, stuff->update);
}

static int
ProcCompositeUnredirectWindow (ClientPtr client)
{
    WindowPtr	pWin;
    REQUEST(xCompositeUnredirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }
    return compUnredirectWindow (client, pWin, stuff->update);
}

static int
ProcCompositeUnredirectSubwindows (ClientPtr client)
{
    WindowPtr	pWin;
    REQUEST(xCompositeUnredirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }
    return compUnredirectSubwindows (client, pWin, stuff->update);
}

static int
ProcCompositeCreateRegionFromBorderClip (ClientPtr client)
{
    WindowPtr	    pWin;
    CompWindowPtr   cw;
    RegionPtr	    pBorderClip, pRegion;
    int rc;
    REQUEST(xCompositeCreateRegionFromBorderClipReq);

    REQUEST_SIZE_MATCH(xCompositeCreateRegionFromBorderClipReq);
    rc = dixLookupResourceByType((pointer *)&pWin, stuff->window, RT_WINDOW, client,
			   DixGetAttrAccess);
    if (rc != Success)
    {
	client->errorValue = stuff->window;
	return (rc == BadValue) ? BadWindow : rc;
    }
    
    LEGAL_NEW_RESOURCE (stuff->region, client);
    
    cw = GetCompWindow (pWin);
    if (cw)
	pBorderClip = &cw->borderClip;
    else
	pBorderClip = &pWin->borderClip;
    pRegion = XFixesRegionCopy (pBorderClip);
    if (!pRegion)
	return BadAlloc;
    REGION_TRANSLATE (pScreen, pRegion, -pWin->drawable.x, -pWin->drawable.y);
    
    if (!AddResource (stuff->region, RegionResType, (pointer) pRegion))
	return BadAlloc;

    return(client->noClientException);
}

static int
ProcCompositeNameWindowPixmap (ClientPtr client)
{
    WindowPtr	    pWin;
    CompWindowPtr   cw;
    PixmapPtr	    pPixmap;
    int rc;
    REQUEST(xCompositeNameWindowPixmapReq);

    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);
    rc = dixLookupResourceByType((pointer *)&pWin, stuff->window, RT_WINDOW, client,
			   DixGetAttrAccess);
    if (rc != Success)
    {
	client->errorValue = stuff->window;
	return (rc == BadValue) ? BadWindow : rc;
    }

    if (!pWin->viewable)
	return BadMatch;

    LEGAL_NEW_RESOURCE (stuff->pixmap, client);
    
    cw = GetCompWindow (pWin);
    if (!cw)
	return BadMatch;

    pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);
    if (!pPixmap)
	return BadMatch;

    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pixmap, RT_PIXMAP,
		  pPixmap, RT_WINDOW, pWin, DixCreateAccess);
    if (rc != Success)
	return rc;

    ++pPixmap->refcnt;

    if (!AddResource (stuff->pixmap, RT_PIXMAP, (pointer) pPixmap))
	return BadAlloc;

    return(client->noClientException);
}


static int
ProcCompositeGetOverlayWindow (ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq); 
    xCompositeGetOverlayWindowReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    CompScreenPtr cs;
    CompOverlayClientPtr pOc;
    int rc;

    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);
    rc = dixLookupResourceByType((pointer *)&pWin, stuff->window, RT_WINDOW, client,
			   DixGetAttrAccess);
    if (rc != Success)
    {
	client->errorValue = stuff->window;
	return (rc == BadValue) ? BadWindow : rc;
    }
    pScreen = pWin->drawable.pScreen;

    /* 
     * Create an OverlayClient structure to mark this client's
     * interest in the overlay window
     */
    pOc = compCreateOverlayClient(pScreen, client);
    if (pOc == NULL)
	return BadAlloc;

    /*
     * Make sure the overlay window exists
     */
    cs = GetCompScreen(pScreen);
    if (cs->pOverlayWin == NULL)
	if (!compCreateOverlayWindow(pScreen))
	{
	    FreeResource (pOc->resource, RT_NONE);
	    return BadAlloc;
	}

    rc = XaceHook(XACE_RESOURCE_ACCESS, client, cs->pOverlayWin->drawable.id,
		  RT_WINDOW, cs->pOverlayWin, RT_NONE, NULL, DixGetAttrAccess);
    if (rc != Success)
    {
	FreeResource (pOc->resource, RT_NONE);
	return rc;
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.overlayWin = cs->pOverlayWin->drawable.id;

    if (client->swapped)
    {
	int n;
	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.overlayWin, n);
    }
    (void) WriteToClient(client, sz_xCompositeGetOverlayWindowReply, (char *)&rep);

    return client->noClientException;
}

static int
ProcCompositeReleaseOverlayWindow (ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq); 
    WindowPtr pWin;
    ScreenPtr pScreen;
    CompOverlayClientPtr pOc;

    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }
    pScreen = pWin->drawable.pScreen;

    /* 
     * Has client queried a reference to the overlay window
     * on this screen? If not, generate an error.
     */
    pOc = compFindOverlayClient (pWin->drawable.pScreen, client);
    if (pOc == NULL)
	return BadMatch;

    /* The delete function will free the client structure */
    FreeResource (pOc->resource, RT_NONE);

    return client->noClientException;
}

static int (*ProcCompositeVector[CompositeNumberRequests])(ClientPtr) = {
    ProcCompositeQueryVersion,
    ProcCompositeRedirectWindow,
    ProcCompositeRedirectSubwindows,
    ProcCompositeUnredirectWindow,
    ProcCompositeUnredirectSubwindows,
    ProcCompositeCreateRegionFromBorderClip,
    ProcCompositeNameWindowPixmap,
    ProcCompositeGetOverlayWindow,
    ProcCompositeReleaseOverlayWindow,
};

static int
ProcCompositeDispatch (ClientPtr client)
{
    REQUEST(xReq);
    
    if (stuff->data < CompositeNumberRequests)
	return (*ProcCompositeVector[stuff->data]) (client);
    else
	return BadRequest;
}

static int
SProcCompositeQueryVersion (ClientPtr client)
{
    int n;
    REQUEST(xCompositeQueryVersionReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeQueryVersionReq);
    swapl(&stuff->majorVersion, n);
    swapl(&stuff->minorVersion, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeRedirectWindow (ClientPtr client)
{
    int n;
    REQUEST(xCompositeRedirectWindowReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);
    swapl (&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeRedirectSubwindows (ClientPtr client)
{
    int n;
    REQUEST(xCompositeRedirectSubwindowsReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);
    swapl (&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeUnredirectWindow (ClientPtr client)
{
    int n;
    REQUEST(xCompositeUnredirectWindowReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);
    swapl (&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeUnredirectSubwindows (ClientPtr client)
{
    int n;
    REQUEST(xCompositeUnredirectSubwindowsReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);
    swapl (&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeCreateRegionFromBorderClip (ClientPtr client)
{
    int n;
    REQUEST(xCompositeCreateRegionFromBorderClipReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeCreateRegionFromBorderClipReq);
    swapl (&stuff->region, n);
    swapl (&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeNameWindowPixmap (ClientPtr client)
{
    int n;
    REQUEST(xCompositeNameWindowPixmapReq);

    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);
    swapl (&stuff->window, n);
    swapl (&stuff->pixmap, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeGetOverlayWindow (ClientPtr client)
{
    int n;
    REQUEST(xCompositeGetOverlayWindowReq);

    swaps (&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);
    swapl(&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeReleaseOverlayWindow (ClientPtr client)
{
    int n;
    REQUEST(xCompositeReleaseOverlayWindowReq);

    swaps (&stuff->length, n);
    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);
    swapl(&stuff->window, n);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int (*SProcCompositeVector[CompositeNumberRequests])(ClientPtr) = {
    SProcCompositeQueryVersion,
    SProcCompositeRedirectWindow,
    SProcCompositeRedirectSubwindows,
    SProcCompositeUnredirectWindow,
    SProcCompositeUnredirectSubwindows,
    SProcCompositeCreateRegionFromBorderClip,
    SProcCompositeNameWindowPixmap,
    SProcCompositeGetOverlayWindow,
    SProcCompositeReleaseOverlayWindow,
};

static int
SProcCompositeDispatch (ClientPtr client)
{
    REQUEST(xReq);
    
    if (stuff->data < CompositeNumberRequests)
	return (*SProcCompositeVector[stuff->data]) (client);
    else
	return BadRequest;
}

void
CompositeExtensionInit (void)
{
    ExtensionEntry  *extEntry;
    int		    s;

    /* Assume initialization is going to fail */
    noCompositeExtension = TRUE;

    for (s = 0; s < screenInfo.numScreens; s++) {
	ScreenPtr pScreen = screenInfo.screens[s];
	VisualPtr vis;

	/* Composite on 8bpp pseudocolor root windows appears to fail, so
	 * just disable it on anything pseudocolor for safety.
	 */
	for (vis = pScreen->visuals; vis->vid != pScreen->rootVisual; vis++)
	    ;
	if ((vis->class | DynamicClass) == PseudoColor)
	    return;

	/* Ensure that Render is initialized, which is required for automatic
	 * compositing.
	 */
	if (GetPictureScreenIfSet(pScreen) == NULL)
	    return;
    }
#ifdef PANORAMIX
    /* Xinerama's rewriting of window drawing before Composite gets to it
     * breaks Composite.
     */
    if (!noPanoramiXExtension)
	return;
#endif

    CompositeClientWindowType = CreateNewResourceType (FreeCompositeClientWindow);
    if (!CompositeClientWindowType)
	return;

    CompositeClientSubwindowsType = CreateNewResourceType (FreeCompositeClientSubwindows);
    if (!CompositeClientSubwindowsType)
	return;

    CompositeClientOverlayType = CreateNewResourceType (FreeCompositeClientOverlay);
    if (!CompositeClientOverlayType)
	return;

    if (!dixRequestPrivate(CompositeClientPrivateKey,
			   sizeof(CompositeClientRec)))
	return;
    if (!AddCallback (&ClientStateCallback, CompositeClientCallback, 0))
	return;

    extEntry = AddExtension (COMPOSITE_NAME, 0, 0,
			     ProcCompositeDispatch, SProcCompositeDispatch,
			     NULL, StandardMinorOpcode);
    if (!extEntry)
	return;
    CompositeReqCode = (CARD8) extEntry->base;

    for (s = 0; s < screenInfo.numScreens; s++)
	if (!compScreenInit (screenInfo.screens[s]))
	    return;
    miRegisterRedirectBorderClipProc (compSetRedirectBorderClip,
				      compGetRedirectBorderClip);

    /* Initialization succeeded */
    noCompositeExtension = FALSE;
}
