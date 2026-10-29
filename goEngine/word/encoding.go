package word

import (
	"golang.org/x/text/encoding/simplifiedchinese"
)

// DecodeGB18030 decodes raw byte slices encoded in GB18030 or GBK into a UTF-8 string.
func DecodeGB18030(src []byte) string {
	if len(src) == 0 {
		return ""
	}
	decoder := simplifiedchinese.GB18030.NewDecoder()
	utf8Bytes, err := decoder.Bytes(src)
	if err != nil {
		// Fallback to GBK in case of specific malformed sequences
		utf8Bytes, err = simplifiedchinese.GBK.NewDecoder().Bytes(src)
		if err != nil {
			return string(src)
		}
	}
	return string(utf8Bytes)
}
