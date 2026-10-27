const physicalKeyMap: Record<string, string> = {
  "1": "A",
  "2": "W",
  "3": "E",
  "4": "F",
  A: "B",
  C: "Z",
  Q: "Q",
  U: "U",
  X: "T",
  Y: "V",
  ",": "N",
  ";": ":",
  ":": ":",
}

export function mapPhysicalKey(key: string) {
  const upperKey = key.toUpperCase()
  if (Object.prototype.hasOwnProperty.call(physicalKeyMap, upperKey)) {
    return physicalKeyMap[upperKey]
  }
  if (/^[A-Z]$/.test(upperKey)) {
    return upperKey.toLowerCase()
  }
  return null
}

export const phoneticKeyboardRows = [
  [
    { label: "A", value: "A" },
    { label: "W", value: "W" },
    { label: "E", value: "E" },
    { label: "F", value: "F" },
  ],
  [
    { label: "Q", value: "Q" },
    { label: "w", value: "w" },
    { label: "e", value: "e" },
    { label: "r", value: "r" },
    { label: "t", value: "t" },
    { label: "V", value: "V" },
    { label: "U", value: "U" },
    { label: "i", value: "i" },
    { label: "C", value: "o" },
    { label: "p", value: "p" },
  ],
  [
    { label: "B", value: "B" },
    { label: "s", value: "s" },
    { label: "d", value: "d" },
    { label: "f", value: "f" },
    { label: "g", value: "g" },
    { label: "h", value: "h" },
    { label: "j", value: "j" },
    { label: "k", value: "k" },
    { label: "l", value: "l" },
    { label: ":", value: ":" },
  ],
  [
    { label: "z", value: "z" },
    { label: "T", value: "T" },
    { label: "Z", value: "Z" },
    { label: "v", value: "v" },
    { label: "b", value: "b" },
    { label: "n", value: "n" },
    { label: "m", value: "m" },
    { label: "N", value: "N" },
  ],
]
