import { normalizePhonetic } from "./phoneticNormalize"

export interface ExampleSentence {
  english: string
  chinese?: string
}

export interface WordSense {
  chinese: string
  english?: string
  examples: ExampleSentence[]
}

export interface PropertySection {
  partOfSpeech: string
  senses: WordSense[]
}

export interface ParsedDefinition {
  phonetics: string[]
  inflections: string[]
  sections: PropertySection[]
  synonyms: string[]
  antonyms: string[]
  relatedWords: string[]
  fallbackText?: string
}

enum DetailType {
  Word = "WORD",
  Property = "PROPERTY",
  Phonetic = "PHONETIC",
  ChMeaning = "CH_MEANING",
  EnMeaning = "EN_MEANING",
  EnSent = "EN_SENT",
  ChSent = "CH_SENT",
  Synonym = "SYNONYM",
  Antonym = "ANTONYM",
  Inflection = "INFLECTION",
}

interface DetailToken {
  type: DetailType
  value: string
}

/**
 * Tokenizes the raw dictionary string based on CDictManager::GetNextDetail logic.
 */
function tokenizeDetail(raw: string): DetailToken[] {
  const tokens: DetailToken[] = []
  const len = raw.length
  let pos = 0

  while (pos < len) {
    const ch = raw[pos]
    let tokenType: DetailType | null = null
    let start = -1

    if (ch === "&") {
      tokenType = DetailType.Word
      start = pos + 1
      pos++
    } else if (ch === "$") {
      tokenType = DetailType.Property
      start = pos + 1
      pos++
    } else if (ch === "@") {
      tokenType = DetailType.Phonetic
      start = pos + 1
      pos++
      // Consume until ']'
      while (pos < len && raw[pos] !== "]") {
        pos++
      }
      if (pos < len && raw[pos] === "]") {
        pos++ // include ']'
      }
    } else if (ch === "<") {
      pos++
      if (pos < len && raw[pos] === "0") {
        tokenType = DetailType.Synonym
        start = pos + 1
        pos++
      } else if (pos < len && raw[pos] === "1") {
        tokenType = DetailType.Antonym
        start = pos + 1
        pos++
      } else {
        tokenType = DetailType.Inflection
        start = pos // Keep following chars
      }
    } else if (ch === "*") {
      tokenType = DetailType.EnMeaning
      start = pos + 1
      pos++
    } else if (ch === "^") {
      tokenType = DetailType.ChMeaning
      start = pos + 1
      pos++
    } else if (ch === "\t") {
      tokenType = DetailType.EnSent
      start = pos + 1
      pos++
    } else if (ch === "|") {
      tokenType = DetailType.ChSent
      start = pos + 1
      pos++
    } else {
      // Unrecognized leading char, consume until next control char
      start = pos
      while (pos < len && !"&$@<*^\t|>".includes(raw[pos])) {
        pos++
      }
      const val = raw.slice(start, pos).trim()
      if (val) {
        tokens.push({ type: DetailType.ChMeaning, value: val })
      }
      continue
    }

    if (tokenType !== null) {
      // Find end of this token (until next control character)
      const valueStart = start
      while (pos < len && !"&$@<*^\t|>".includes(raw[pos])) {
        if (raw[pos] === "%") {
          // Escaped character skip
          pos += 2
        } else {
          pos++
        }
      }
      let val = raw.slice(valueStart, pos)
      // Normalize escape sequences (%xx or %%)
      val = val.replace(/%%/g, "%").trim()
      if (val) {
        tokens.push({ type: tokenType, value: val })
      }
    }
  }

  return tokens
}

/**
 * Splits lists of words (e.g. from <0 or <1 or &), which can be separated by #, comma, or spaces.
 */
function extractWordList(raw: string): string[] {
  return raw
    .split(/[#,;\s]+/)
    .map((w) => w.trim())
    .filter((w) => w.length > 0)
}

/**
 * Extracts phonetic strings, e.g. from `[5Apl]` or `[5Apl,-pl]`, and converts them to Unicode IPA.
 */
function extractPhonetics(raw: string): string[] {
  const clean = raw.replace(/^\[+|\]+$/g, "")
  const parts = clean.split(/[,;\t]+/)
  const result: string[] = []
  for (const part of parts) {
    const trimmed = part.trim()
    if (trimmed) {
      result.push(normalizePhonetic(trimmed))
    }
  }
  return result
}

/**
 * Parses raw simple or detailed definition into a structured representation.
 */
export function parseDefinition(rawText: string | undefined): ParsedDefinition {
  const result: ParsedDefinition = {
    phonetics: [],
    inflections: [],
    sections: [],
    synonyms: [],
    antonyms: [],
    relatedWords: [],
  }

  if (!rawText || !rawText.trim()) {
    return result
  }

  const raw = rawText.trim()
  const tokens = tokenizeDetail(raw)

  if (tokens.length === 0) {
    result.fallbackText = raw
    return result
  }

  let currentSection: PropertySection | null = null
  let currentSense: WordSense | null = null

  function ensureSection(pos = ""): PropertySection {
    if (!currentSection || (pos && currentSection.partOfSpeech !== pos)) {
      currentSection = {
        partOfSpeech: pos,
        senses: [],
      }
      result.sections.push(currentSection)
      currentSense = null
    }
    return currentSection
  }

  function ensureSense(chinese = ""): WordSense {
    const sec = ensureSection()
    if (!currentSense || chinese) {
      currentSense = {
        chinese: chinese,
        examples: [],
      }
      sec.senses.push(currentSense)
    }
    return currentSense
  }

  for (const token of tokens) {
    switch (token.type) {
      case DetailType.Phonetic:
        result.phonetics.push(...extractPhonetics(token.value))
        break

      case DetailType.Inflection: {
        const inf = token.value.replace(/^<+|>+$/g, "").trim()
        if (inf) result.inflections.push(inf)
        break
      }

      case DetailType.Property:
        currentSection = ensureSection(token.value)
        currentSense = null
        break

      case DetailType.ChMeaning:
        ensureSense(token.value)
        break

      case DetailType.EnMeaning: {
        const s = currentSense ?? ensureSense("")
        s.english = token.value
        break
      }

      case DetailType.EnSent: {
        const s = ensureSense("")
        s.examples.push({ english: token.value })
        break
      }

      case DetailType.ChSent: {
        const s = ensureSense("")
        if (s.examples.length > 0) {
          const lastEx = s.examples[s.examples.length - 1]
          if (!lastEx.chinese) {
            lastEx.chinese = token.value
            break
          }
        }
        s.examples.push({ english: "", chinese: token.value })
        break
      }

      case DetailType.Synonym:
        result.synonyms.push(...extractWordList(token.value))
        break

      case DetailType.Antonym:
        result.antonyms.push(...extractWordList(token.value))
        break

      case DetailType.Word:
        result.relatedWords.push(...extractWordList(token.value))
        break
    }
  }

  // Deduplicate phonetics and lists
  result.phonetics = Array.from(new Set(result.phonetics))
  result.synonyms = Array.from(new Set(result.synonyms))
  result.antonyms = Array.from(new Set(result.antonyms))
  result.relatedWords = Array.from(new Set(result.relatedWords))

  // Clean up empty senses if any
  for (const sec of result.sections) {
    sec.senses = sec.senses.filter(
      (s) => s.chinese || s.english || s.examples.length > 0,
    )
  }
  result.sections = result.sections.filter(
    (sec) => sec.partOfSpeech || sec.senses.length > 0,
  )

  return result
}
