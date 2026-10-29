package word

import (
	"fmt"
	"os"
)

type DefinitionStore struct {
	simpleData []byte
	detailData []byte
}

func LoadDefinitions(simplePath, detailPath string) (*DefinitionStore, error) {
	simpleData, err := os.ReadFile(simplePath)
	if err != nil {
		return nil, fmt.Errorf("read simple meaning file %s: %w", simplePath, err)
	}

	detailData, err := os.ReadFile(detailPath)
	if err != nil {
		return nil, fmt.Errorf("read detail meaning file %s: %w", detailPath, err)
	}

	return &DefinitionStore{
		simpleData: simpleData,
		detailData: detailData,
	}, nil
}

func decodeLength(data []byte, offset int) (length int, contentOffset int, ok bool) {
	if offset < 0 || offset >= len(data) {
		return 0, 0, false
	}
	var decoded uint64
	var place uint64 = 1
	for i := 0; i < 4 && offset < len(data); i++ {
		b := data[offset]
		if b == 0 {
			return 0, 0, false
		}
		if b == 0xff {
			contentOffset = offset + 1
			length = int(decoded)
			if length <= len(data)-contentOffset {
				return length, contentOffset, true
			}
			return 0, 0, false
		}
		decoded += uint64(b-1) * place
		place *= 254
		offset++
	}
	return 0, 0, false
}

func (store *DefinitionStore) SimpleMeaning(offset int) string {
	if store == nil || offset < 0 {
		return ""
	}
	length, contentOffset, ok := decodeLength(store.simpleData, offset)
	if !ok || length == 0 {
		return ""
	}
	return DecodeGB18030(store.simpleData[contentOffset : contentOffset+length])
}

func (store *DefinitionStore) DetailMeaning(offset int) string {
	if store == nil || offset < 0 {
		return ""
	}
	length, contentOffset, ok := decodeLength(store.detailData, offset)
	if !ok || length == 0 {
		return ""
	}
	return DecodeGB18030(store.detailData[contentOffset : contentOffset+length])
}
