/**
 * Normalizes legacy dictionary ASCII phonetic encoding to Unicode IPA characters,
 * and re-normalizes Unicode IPA characters back to the internal ASCII encoding for querying.
 *
 * Reference: iOS/OKWord/DictManager/WordManager.cpp (NormalizePhoneticBuffer & ReNormalizePhoneticBuffer)
 */

export function normalizePhonetic(raw: string): string {
  if (!raw) return ""
  let result = ""
  for (let i = 0; i < raw.length; i++) {
    const ch = raw[i]
    switch (ch) {
      case "E":
      case "L":
      case "\\":
        result += "\u0259" // ə (Schwa)
        break
      case "`":
      case "5":
      case "'":
        result += "\u02c8" // ˈ (Primary stress mark)
        break
      case "Z":
        result += "\u025b" // ɛ (Open E)
        break
      case "7":
      case "9":
        result += "\u02cc" // ˌ (Secondary stress mark)
        break
      case "Q":
        result += "\u028c" // ʌ (Turned V)
        break
      case "I":
        result += "i"
        break
      case "A":
        result += "\u00e6" // æ (Ash)
        break
      case "C":
      case "R":
      case "o":
        result += "\u0254" // ɔ (Open O)
        break
      case "U":
      case "J":
      case "?":
        result += "u"
        break
      case "B":
        result += "\u0251" // ɑ (Script A)
        break
      case "N":
        result += "\u014b" // ŋ (Eng)
        break
      case "V":
        result += "\u0292" // ʒ (Ezh)
        break
      case "F":
        result += "\u0283" // ʃ (Esh)
        break
      case "T":
        result += "\u00f0" // ð (Eth)
        break
      case "W":
      case "P":
        result += "\u03b8" // θ (Theta)
        break
      case "g":
      case "^":
        result += "\u0261" // ɡ (Script G)
        break
      default:
        result += ch
        break
    }
  }
  return result
}

export function reNormalizePhonetic(normalized: string): string {
  if (!normalized) return ""
  let result = ""
  for (let i = 0; i < normalized.length; i++) {
    const ch = normalized[i]
    switch (ch) {
      case "\u0259": // ə
        result += "E"
        break
      case "\u025b": // ɛ
        result += "Z"
        break
      case "\u02cc": // ˌ
        result += "7"
        break
      case "\u02c8": // ˈ
      case "`":
      case "5":
      case "'":
        result += "'"
        break
      case "\u028c": // ʌ
        result += "Q"
        break
      case "\u00e6": // æ
        result += "A"
        break
      case "\u0254": // ɔ
        result += "C"
        break
      case "\u0251": // ɑ
        result += "B"
        break
      case "\u014b": // ŋ
        result += "N"
        break
      case "\u0292": // ʒ
        result += "V"
        break
      case "\u0283": // ʃ
        result += "F"
        break
      case "\u00f0": // ð
        result += "T"
        break
      case "\u03b8": // θ
        result += "W"
        break
      case "\u0261": // ɡ
        result += "g"
        break
      case "\u02d0": // ː (Modifier letter triangular colon)
        result += ":"
        break
      default:
        result += ch
        break
    }
  }
  return result
}
