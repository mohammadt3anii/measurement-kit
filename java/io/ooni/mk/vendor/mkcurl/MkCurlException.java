package io.ooni.mk.vendor.mkcurl;

public class MkCurlException extends Exception {
  String cause;

  public MkCurlException(String cause) {
    this.cause = cause;
  }

  @Override public String getMessage() {
    return this.cause;
  }
}
