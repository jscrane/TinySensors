wemo_get()
{
  local host=$1

  cat << EOF |
<?xml version="1.0" encoding="utf-8"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
 s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:GetBinaryState xmlns:u="urn:Belkin:service:basicevent:1"/>
  </s:Body>
</s:Envelope> 
EOF
  curl -A '' -X POST \
  -H 'Content-type: text/xml; charset="utf-8"' \
  -H 'SOAPACTION: "urn:Belkin:service:basicevent:1#GetBinaryState"' \
  -s http://$host:49153/upnp/control/basicevent1 \
  -d@- 2>&1 | sed -n 's/<BinaryState>\(.\)<\/BinaryState>/\1/p'
}

wemo_set() 
{
  local host=$1
  local value=$2

  cat << EOF |
<?xml version="1.0" encoding="utf-8"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
 s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:SetBinaryState xmlns:u="urn:Belkin:service:basicevent:1">
      <BinaryState>$value</BinaryState>
     </u:SetBinaryState>
  </s:Body>
</s:Envelope>
EOF
  curl -A '' -X POST \
  -H 'Content-type: text/xml; charset="utf-8"' \
  -H 'SOAPACTION: "urn:Belkin:service:basicevent:1#SetBinaryState"' \
  -s http://$host:49153/upnp/control/basicevent1 \
  -o /dev/null \
  -d@-
}
