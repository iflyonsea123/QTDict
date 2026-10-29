const physicalKeyMap: Record<string, string> = {
  "1": "\u00e6", // æ (Ash)
  "2": "\u03b8", // θ (Theta)
  "3": "\u0259", // ə (Schwa)
  "4": "\u0283", // ʃ (Esh)
  A: "\u0251",   // ɑ (Script A)
  C: "\u025b",   // ɛ (Open E)
  Q: "\u028c",   // ʌ (Turned V)
  U: "u",
  X: "\u00f0",   // ð (Eth)
  Y: "\u0292",   // ʒ (Ezh)
  ",": "\u014b", // ŋ (Eng)
  ";": ":",
  ":": ":",
}

export function mapPhysicalKey(key: string) {
  const upperKey = key.toUpperCase()
  if (Object.prototype.hasOwnProperty.call(physicalKeyMap, upperKey)) {
    return physicalKeyMap[upperKey]
  }
  if (/^[A-Z]$/.test(upperKey)) {
    const lower = upperKey.toLowerCase()
    if (lower === "g") {
      return "\u0261" // ɡ
    }
    return lower
  }
  return null
}

export const phoneticKeyboardRows = [
  [
    { label: "ˈ", value: "ˈ" },
    { label: "ˌ", value: "ˌ" },
    { label: "æ", value: "\u00e6" },
    { label: "θ", value: "\u03b8" },
    { label: "ə", value: "\u0259" },
    { label: "ʃ", value: "\u0283" },
  ],
  [
    { label: "ʌ", value: "\u028c" },
    { label: "w", value: "w" },
    { label: "e", value: "e" },
    { label: "r", value: "r" },
    { label: "t", value: "t" },
    { label: "ʒ", value: "\u0292" },
    { label: "u", value: "u" },
    { label: "i", value: "i" },
    { label: "ɔ", value: "\u0254" },
    { label: "p", value: "p" },
  ],
  [
    { label: "ɑ", value: "\u0251" },
    { label: "s", value: "s" },
    { label: "d", value: "d" },
    { label: "f", value: "f" },
    { label: "ɡ", value: "\u0261" },
    { label: "h", value: "h" },
    { label: "j", value: "j" },
    { label: "k", value: "k" },
    { label: "l", value: "l" },
    { label: "ː", value: ":" },
  ],
  [
    { label: "z", value: "z" },
    { label: "ð", value: "\u00f0" },
    { label: "ɛ", value: "\u025b" },
    { label: "v", value: "v" },
    { label: "b", value: "b" },
    { label: "n", value: "n" },
    { label: "m", value: "m" },
    { label: "ŋ", value: "\u014b" },
  ],
]
