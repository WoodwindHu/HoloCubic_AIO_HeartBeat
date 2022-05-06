#include "IPGWClient.h"

const char *test_root_ca = "-----BEGIN CERTIFICATE-----\n" \
                        "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
                        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
                        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
                        "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
                        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
                        "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
                        "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
                        "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
                        "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
                        "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
                        "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
                        "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
                        "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
                        "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
                        "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
                        "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
                        "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
                        "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
                        "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
                        "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
                        "-----END CERTIFICATE-----\n";

IPGWClient::IPGWClient(const String _username, const String _password, const String _mac) {
  IPGW_DEBUG("Initializing IPGWClient object.");
  IPGW_DEBUG("Username is: " + _username + "; Password is: " + _password);
  username = _username;
  password = _password;
  IPGW_DEBUG("MAC address for NodeMCU board STA mode is: " + _mac);
  mac = _mac;
  mac.replace(":", "-");
  mac.toLowerCase();
  IPGW_DEBUG("MAC address reformatted: " + mac);
  IPGW_DEBUG("Initialization complete.");
}

IPGWStatus IPGWClient::connect() {
  client.setCACert(test_root_ca);
  if (!client.connect(host.c_str(), 443)) {
    IPGW_DEBUG("Connection failed");
    return IPGW_NO_CONN;
  }
  #ifdef IPGW_CHECK_CERT
  if (client.verify(fingerprint.c_str(), host.c_str())) {
    IPGW_DEBUG("Certificate matches.");
  } else {
    IPGW_DEBUG("Certificate doesn't match");
    return IPGW_CERT_ERR;
  }
  #else
    IPGW_DEBUG("Certificate check skipped.");
  #endif
  String payload = "cmd=open&username=" + username + "&password=" + password + "&iprange=free&ip=&lang=en&app=1.0";
  int content_length =  payload.length();
  String POST_payload = String("POST ") + endpoint + " HTTP/1.0\r\n" +
                    UserAgent_prefix + mac + "\r\n" + header + "Content-Length: " +
                    String(content_length) + "\r\n\r\n" + payload;
  IPGW_DEBUG("The POSTed message is: " + POST_payload);
  client.print(POST_payload.c_str());
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      IPGW_DEBUG("Headers received from PKU ITS.");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  IPGW_DEBUG("Received response body is: " + line);
  if (line.startsWith("{\"succ\":\"\"")) {
    IPGW_DEBUG("Successfully authenticated PKU ITS.");
    return IPGW_SUCC;
  }
  else if (line.startsWith("{\"error\":\"Password error\"")) {
    IPGW_DEBUG("Wrong password!");
    return IPGW_PWD_ERR;
  }
  else {
    IPGW_DEBUG("Something Wrong! Unable to authenticated PKU ITS.");
    return IPGW_GENERAL_ERR;
  }
}
