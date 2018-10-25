package io.ooni.mk.vendor.mkcurl;

class FFI {
  protected static native long RequestNew();

  protected static native void RequestSetCABundlePath(long handle, String path);

  protected static native void RequestEnableHTTP2(long handle);

  protected static native void RequestSetMethodPOST(long handle);

  protected static native void RequestSetURL(long handle, String url);

  protected static native void RequestAddHeader(long handle, String header);

  protected static native void RequestSetBody(long handle, byte[] body);

  protected static native void RequestSetTimeout(long handle, long timeout);

  protected static native void RequestSetProxyURL(long hande, String url);

  protected static native void RequestEnableFollowRedirect(long handle);

  protected static native void RequestDelete(long handle);

  protected static native long ResponseGetError(long handle);

  protected static native String ResponseGetRedirectURL(long handle);

  protected static native byte[] ResponseGetBody(long handle);

  protected static native double ResponseGetBytesSent(long handle);

  protected static native double ResponseGetBytesRecv(long handle);

  protected static native byte[] ResponseGetLogs(long handle);

  protected static native String ResponseGetRequestHeaders(long handle);

  protected static native byte[] ResponseGetResponseHeaders(long handle);

  protected static native String ResponseGetCertificateChain(long handle);

  protected static native void ResponseDelete(long handle);

  protected static native long Perform(long handle);
}
