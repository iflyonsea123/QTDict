export interface PhoneticEntry {
  index: number
  pronunciation: string
  word: string
}

export interface PhoneticSearchResult extends PhoneticEntry {
  exact: boolean
}

export interface WordEntry {
  index: number
  word: string
  simple_meaning?: string
  detail_meaning?: string
}

export interface WordSearchResult extends WordEntry {
  exact: boolean
}

async function request(path: string, signal?: AbortSignal): Promise<unknown> {
  let response: Response
  try {
    response = await fetch(path, {
      headers: { Accept: "application/json" },
      signal,
    })
  } catch (error) {
    if (error instanceof DOMException && error.name === "AbortError") {
      throw error
    }
    throw new Error("无法连接词典服务，请确认 Go 服务已启动。")
  }

  const body: unknown = await response.json().catch(() => null)
  if (!response.ok) {
    const message =
      typeof body === "object" &&
      body !== null &&
      "error" in body &&
      typeof body.error === "string"
        ? body.error
        : `词典服务请求失败（HTTP ${response.status}）`
    throw new Error(message)
  }

  return body
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null
}

function parseEntry(value: unknown): PhoneticEntry {
  if (
    !isRecord(value) ||
    typeof value.index !== "number" ||
    !Number.isInteger(value.index) ||
    typeof value.pronunciation !== "string" ||
    typeof value.word !== "string"
  ) {
    throw new Error("词典服务返回的词条格式无效。")
  }
  return {
    index: value.index,
    pronunciation: value.pronunciation,
    word: value.word,
  }
}

export async function getEntryCount(signal?: AbortSignal) {
  const result = await request("/api/phonetic/count", signal)
  if (
    !isRecord(result) ||
    typeof result.count !== "number" ||
    !Number.isInteger(result.count) ||
    result.count < 0
  ) {
    throw new Error("词典服务返回的词条总数格式无效。")
  }
  return result.count
}

export async function getEntry(index: number, signal?: AbortSignal) {
  const result = await request(`/api/phonetic/entries/${index}`, signal)
  return parseEntry(result)
}

export async function searchPhonetic(query: string, signal?: AbortSignal) {
  const params = new URLSearchParams({ q: query })
  const result = await request(
    `/api/phonetic/search?${params.toString()}`,
    signal,
  )
  const entry = parseEntry(result)
  if (!isRecord(result) || typeof result.exact !== "boolean") {
    throw new Error("词典服务返回的搜索结果格式无效。")
  }
  return { ...entry, exact: result.exact } satisfies PhoneticSearchResult
}

function parseWordEntry(value: unknown): WordEntry {
  if (
    !isRecord(value) ||
    typeof value.index !== "number" ||
    !Number.isInteger(value.index) ||
    typeof value.word !== "string"
  ) {
    throw new Error("词典服务返回的单词条目格式无效。")
  }
  return {
    index: value.index,
    word: value.word,
    simple_meaning: typeof value.simple_meaning === "string" ? value.simple_meaning : "",
    detail_meaning: typeof value.detail_meaning === "string" ? value.detail_meaning : "",
  }
}

export async function getWordCount(signal?: AbortSignal): Promise<number> {
  const result = await request("/api/word/count", signal)
  if (
    !isRecord(result) ||
    typeof result.count !== "number" ||
    !Number.isInteger(result.count) ||
    result.count < 0
  ) {
    throw new Error("词典服务返回的单词总数格式无效。")
  }
  return result.count
}

export async function getWordEntry(index: number, signal?: AbortSignal): Promise<WordEntry> {
  const result = await request(`/api/word/entries/${index}`, signal)
  return parseWordEntry(result)
}

export async function searchWord(query: string, signal?: AbortSignal): Promise<WordSearchResult> {
  const params = new URLSearchParams({ q: query })
  const result = await request(
    `/api/word/search?${params.toString()}`,
    signal,
  )
  const entry = parseWordEntry(result)
  if (!isRecord(result) || typeof result.exact !== "boolean") {
    throw new Error("词典服务返回的搜索结果格式无效。")
  }
  return { ...entry, exact: result.exact } satisfies WordSearchResult
}
