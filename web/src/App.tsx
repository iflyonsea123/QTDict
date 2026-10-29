import { useEffect, useRef, useState } from "react"
import type { FormEvent, KeyboardEvent } from "react"
import {
  BookOpen,
  ChevronLeft,
  ChevronRight,
  Command,
  Keyboard,
  Search,
  ShieldCheck,
  Volume2,
} from "lucide-react"
import {
  getEntry,
  getEntryCount,
  getWordCount,
  getWordEntry,
  searchPhonetic,
  searchWord,
} from "./api"
import type { PhoneticEntry, WordEntry } from "./api"
import { DefinitionView } from "./DefinitionView"
import { mapPhysicalKey, phoneticKeyboardRows } from "./phoneticKeys"
import { normalizePhonetic, reNormalizePhonetic } from "./phoneticNormalize"

type Mode = "word" | "phonetic"
type SearchState = "idle" | "loading" | "error" | "ready"

function formatCount(value: number | null) {
  return value === null ? "加载中…" : new Intl.NumberFormat("zh-CN").format(value)
}

export default function App() {
  const [mode, setMode] = useState<Mode>("word")

  // Word Mode State
  const [wordQuery, setWordQuery] = useState("")
  const [wordCount, setWordCount] = useState<number | null>(null)
  const [selectedWord, setSelectedWord] = useState<WordEntry | null>(null)
  const [wordExact, setWordExact] = useState<boolean | null>(null)
  const [wordState, setWordState] = useState<SearchState>("idle")
  const [wordError, setWordError] = useState("")
  const wordInputRef = useRef<HTMLInputElement>(null)

  // Phonetic Mode State
  const [phoneticQuery, setPhoneticQuery] = useState("")
  const [phoneticCount, setPhoneticCount] = useState<number | null>(null)
  const [selectedPhonetic, setSelectedPhonetic] = useState<PhoneticEntry | null>(null)
  const [phoneticExact, setPhoneticExact] = useState<boolean | null>(null)
  const [phoneticState, setPhoneticState] = useState<SearchState>("idle")
  const [phoneticError, setPhoneticError] = useState("")
  const [keyboardOpen, setKeyboardOpen] = useState(true)
  const phoneticInputRef = useRef<HTMLInputElement>(null)

  const requestRef = useRef<AbortController | null>(null)

  useEffect(() => {
    const controller = new AbortController()
    getWordCount(controller.signal)
      .then(setWordCount)
      .catch(() => {
        // phonetic fallback handled below
      })
    getEntryCount(controller.signal)
      .then(setPhoneticCount)
      .catch(() => {
        // fallback
      })
    return () => controller.abort()
  }, [])

  useEffect(() => () => requestRef.current?.abort(), [])

  // Phonetic Virtual Keyboard Insertion
  function insertPhoneticKey(value: string) {
    const input = phoneticInputRef.current
    if (!input) return
    const start = input.selectionStart ?? phoneticQuery.length
    const end = input.selectionEnd ?? phoneticQuery.length
    const nextQuery = `${phoneticQuery.slice(0, start)}${value}${phoneticQuery.slice(end)}`
    setPhoneticQuery(nextQuery)
    requestAnimationFrame(() => {
      input.focus()
      const caret = start + value.length
      input.setSelectionRange(caret, caret)
    })
  }

  function handlePhysicalKey(event: KeyboardEvent<HTMLInputElement>) {
    if (
      event.ctrlKey ||
      event.metaKey ||
      event.altKey ||
      event.key.length !== 1
    ) {
      return
    }
    event.preventDefault()
    const mapped = mapPhysicalKey(event.key)
    if (mapped) insertPhoneticKey(mapped)
  }

  // Handle Word Search
  async function handleWordSearch(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    const value = wordQuery.trim()
    if (!value) {
      setWordError("请输入要查询的单词。")
      setWordState("error")
      wordInputRef.current?.focus()
      return
    }

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setWordState("loading")
    setWordError("")
    try {
      const result = await searchWord(value, controller.signal)
      setSelectedWord({
        index: result.index,
        word: result.word,
        simple_meaning: result.simple_meaning,
        detail_meaning: result.detail_meaning,
      })
      setWordExact(result.exact)
      setWordState("ready")
    } catch (error) {
      if (error instanceof DOMException && error.name === "AbortError") return
      setWordError(error instanceof Error ? error.message : "单词搜索失败。")
      setWordState("error")
    }
  }

  // Handle Phonetic Search
  async function handlePhoneticSearch(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    const value = phoneticQuery.trim()
    if (!value) {
      setPhoneticError("请输入或选择要查询的音标。")
      setPhoneticState("error")
      phoneticInputRef.current?.focus()
      return
    }

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setPhoneticState("loading")
    setPhoneticError("")
    try {
      const internalQuery = reNormalizePhonetic(value)
      const result = await searchPhonetic(internalQuery, controller.signal)
      const normalizedPron = normalizePhonetic(result.pronunciation)
      setSelectedPhonetic({
        index: result.index,
        pronunciation: normalizedPron,
        word: result.word,
      })
      setPhoneticExact(result.exact)
      setPhoneticState("ready")
    } catch (error) {
      if (error instanceof DOMException && error.name === "AbortError") return
      setPhoneticError(error instanceof Error ? error.message : "音标搜索失败。")
      setPhoneticState("error")
    }
  }

  // Step entry navigation
  async function moveWordEntry(offset: -1 | 1) {
    if (!selectedWord || !wordCount) return
    const nextIndex = selectedWord.index + offset
    if (nextIndex < 1 || nextIndex > wordCount) return

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setWordState("loading")
    setWordError("")
    try {
      const entry = await getWordEntry(nextIndex, controller.signal)
      setSelectedWord(entry)
      setWordExact(null)
      setWordQuery(entry.word)
      setWordState("ready")
      wordInputRef.current?.focus()
    } catch (error) {
      if (error instanceof DOMException && error.name === "AbortError") return
      setWordError(error instanceof Error ? error.message : "读取单词条目失败。")
      setWordState("error")
    }
  }

  async function movePhoneticEntry(offset: -1 | 1) {
    if (!selectedPhonetic || !phoneticCount) return
    const nextIndex = selectedPhonetic.index + offset
    if (nextIndex < 1 || nextIndex > phoneticCount) return

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setPhoneticState("loading")
    setPhoneticError("")
    try {
      const entry = await getEntry(nextIndex, controller.signal)
      const normalizedPron = normalizePhonetic(entry.pronunciation)
      setSelectedPhonetic({
        ...entry,
        pronunciation: normalizedPron,
      })
      setPhoneticExact(null)
      setPhoneticQuery(normalizedPron)
      setPhoneticState("ready")
      phoneticInputRef.current?.focus()
    } catch (error) {
      if (error instanceof DOMException && error.name === "AbortError") return
      setPhoneticError(error instanceof Error ? error.message : "读取音标词条失败。")
      setPhoneticState("error")
    }
  }

  // Handle clicking a phonetic badge from word definition: switch to phonetic tab & search
  function handleSelectPhonetic(pron: string) {
    setMode("phonetic")
    setPhoneticQuery(pron)

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setPhoneticState("loading")
    setPhoneticError("")

    const internalQuery = reNormalizePhonetic(pron)
    searchPhonetic(internalQuery, controller.signal)
      .then((result) => {
        const normalizedPron = normalizePhonetic(result.pronunciation)
        setSelectedPhonetic({
          index: result.index,
          pronunciation: normalizedPron,
          word: result.word,
        })
        setPhoneticExact(result.exact)
        setPhoneticState("ready")
      })
      .catch((error) => {
        if (error instanceof DOMException && error.name === "AbortError") return
        setPhoneticError(error instanceof Error ? error.message : "音标搜索失败。")
        setPhoneticState("error")
      })
  }

  // Handle clicking a word link tag (synonym, antonym, or related word): search word directly
  function handleSelectWord(wordToSearch: string) {
    setWordQuery(wordToSearch)

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setWordState("loading")
    setWordError("")

    searchWord(wordToSearch, controller.signal)
      .then((result) => {
        setSelectedWord({
          index: result.index,
          word: result.word,
          simple_meaning: result.simple_meaning,
          detail_meaning: result.detail_meaning,
        })
        setWordExact(result.exact)
        setWordState("ready")
      })
      .catch((error) => {
        if (error instanceof DOMException && error.name === "AbortError") return
        setWordError(error instanceof Error ? error.message : "单词搜索失败。")
        setWordState("error")
      })
  }

  const isConnected = wordCount !== null || phoneticCount !== null

  return (
    <div className="app-shell">
      <aside className="sidebar">
        <a className="brand" href="/" aria-label="Okeyen Dictionary 首页">
          <span className="brand-icon"><BookOpen size={21} /></span>
          <span>
            <strong>Okeyen</strong>
            <small>DICTIONARY WORKSPACE</small>
          </span>
        </a>
        <div className="nav-section-label">词典功能</div>
        <nav aria-label="词典导航">
          <button
            type="button"
            className={`nav-link ${mode === "word" ? "active" : ""}`}
            onClick={() => setMode("word")}
          >
            <BookOpen size={17} />
            <span>单词查询</span>
            <ChevronRight className="nav-chevron" size={15} />
          </button>
          <button
            type="button"
            className={`nav-link ${mode === "phonetic" ? "active" : ""}`}
            onClick={() => setMode("phonetic")}
          >
            <Volume2 size={17} />
            <span>音标索引</span>
            <ChevronRight className="nav-chevron" size={15} />
          </button>
          <span className="nav-link disabled" aria-disabled="true">
            <Command size={17} />
            <span>词根词缀</span>
            <small>即将推出</small>
          </span>
        </nav>
        <div className="sidebar-bottom">
          <div className="privacy-card">
            <ShieldCheck size={17} />
            <div>
              <strong>你的词典服务</strong>
              <span>搜索请求发送至 Go 服务</span>
            </div>
          </div>
          <p className="sidebar-version">Okeyen Dictionary · Web</p>
        </div>
      </aside>

      <main className="main-area">
        <header className="topbar">
          <div className="breadcrumb">
            <span>词典工作台</span>
            <ChevronRight size={14} />
            <strong>{mode === "word" ? "单词查询" : "音标索引"}</strong>
          </div>
          <span className="service-indicator">
            <i className={!isConnected ? "connecting" : ""} />
            {!isConnected ? "连接服务中" : "词典服务已连接"}
          </span>
        </header>

        <div className="page-content">
          {mode === "word" ? (
            /* --- 单词查询模块 --- */
            <>
              <section className="page-heading">
                <div>
                  <p className="eyebrow">ENGLISH DICTIONARY</p>
                  <h1>单词词典</h1>
                  <p className="heading-copy">
                    查询英文单词，快速获取简明词义与详细释义。
                  </p>
                </div>
                <div className="count-card" aria-live="polite">
                  <span>总收录词汇</span>
                  <strong>{formatCount(wordCount)}</strong>
                </div>
              </section>

              <section className="workspace-card" aria-labelledby="word-search-title">
                <div className="section-heading">
                  <div className="section-icon"><Search size={18} /></div>
                  <div>
                    <h2 id="word-search-title">单词检索</h2>
                    <p>支持英文字符输入，按旧版词典顺序进行精确与最近条目检索。</p>
                  </div>
                </div>

                <form
                  id="word-search-form"
                  className="search-form"
                  onSubmit={handleWordSearch}
                >
                  <label className="sr-only" htmlFor="word-query">单词查询</label>
                  <div className="search-input-wrap">
                    <Search className="input-sound-icon" size={19} />
                    <input
                      ref={wordInputRef}
                      id="word-query"
                      className="word-input"
                      autoComplete="off"
                      spellCheck={false}
                      value={wordQuery}
                      onChange={(event) => setWordQuery(event.target.value)}
                      placeholder="输入要查询的单词，例如：apple, book, computer"
                      aria-describedby="word-input-help"
                    />
                    {wordQuery && (
                      <button
                        className="clear-button"
                        type="button"
                        onClick={() => {
                          setWordQuery("")
                          setWordState("idle")
                          setWordError("")
                          wordInputRef.current?.focus()
                        }}
                        aria-label="清空输入"
                      >
                        清空
                      </button>
                    )}
                  </div>
                  <button className="search-button" type="submit" disabled={wordState === "loading"}>
                    <Search size={17} />
                    {wordState === "loading" ? "查询中…" : "搜索单词"}
                  </button>
                </form>
                <div className="search-footnote" id="word-input-help">
                  <span>回车或点击按钮即可搜索；支持不区分大小写。</span>
                </div>
              </section>

              {wordError && (
                <div className="alert-error" role="alert">
                  <strong>暂时无法完成操作</strong>
                  <span>{wordError}</span>
                </div>
              )}

              <section className="result-card" aria-labelledby="word-result-title" aria-live="polite">
                <div className="result-header">
                  <div>
                    <p className="eyebrow">WORD DEFINITIONS</p>
                    <h2 id="word-result-title">词义信息</h2>
                  </div>
                  {selectedWord && wordExact !== null && (
                    <span className={`match-badge ${wordExact ? "exact" : "approximate"}`}>
                      <i />
                      {wordExact ? "精确匹配" : "最近匹配"}
                    </span>
                  )}
                </div>

                {selectedWord ? (
                  <>
                    <div className="entry-primary">
                      <div>
                        <span className="field-label">目标单词</span>
                        <h3 className="entry-word-headline">{selectedWord.word}</h3>
                      </div>
                      <span className="entry-number">
                        索引 #{selectedWord.index.toLocaleString("zh-CN")}
                      </span>
                    </div>

                    <div className="definition-section">
                      <DefinitionView
                        title="简明词义"
                        rawText={selectedWord.simple_meaning}
                        onSelectPhonetic={handleSelectPhonetic}
                        onSelectWord={handleSelectWord}
                      />

                      <DefinitionView
                        title="详细释义"
                        rawText={selectedWord.detail_meaning}
                        onSelectPhonetic={handleSelectPhonetic}
                        onSelectWord={handleSelectWord}
                      />
                    </div>

                    <div className="entry-navigation">
                      <span>
                        {selectedWord.index.toLocaleString("zh-CN")} / {formatCount(wordCount)}
                      </span>
                      <div>
                        <button
                          type="button"
                          className="icon-control"
                          onClick={() => moveWordEntry(-1)}
                          disabled={selectedWord.index <= 1 || wordState === "loading"}
                          aria-label="上一个单词"
                          title="上一个单词"
                        >
                          <ChevronLeft size={18} />
                        </button>
                        <button
                          type="button"
                          className="icon-control"
                          onClick={() => moveWordEntry(1)}
                          disabled={wordCount === null || selectedWord.index >= wordCount || wordState === "loading"}
                          aria-label="下一个单词"
                          title="下一个单词"
                        >
                          <ChevronRight size={18} />
                        </button>
                      </div>
                    </div>
                  </>
                ) : (
                  <div className="empty-result">
                    <span className="empty-icon"><BookOpen size={22} /></span>
                    <strong>搜索结果将在这里显示</strong>
                    <p>在上方输入单词后查询，可查看简明词义、详细释义与前后索引词条。</p>
                  </div>
                )}
              </section>
            </>
          ) : (
            /* --- 音标索引模块 --- */
            <>
              <section className="page-heading">
                <div>
                  <p className="eyebrow">PHONETIC INDEX</p>
                  <h1>音标词典</h1>
                  <p className="heading-copy">
                    输入音标，按旧版词典排序规则查找对应词条。
                  </p>
                </div>
                <div className="count-card" aria-live="polite">
                  <span>索引词条</span>
                  <strong>{formatCount(phoneticCount)}</strong>
                </div>
              </section>

              <section className="workspace-card" aria-labelledby="phonetic-search-title">
                <div className="section-heading">
                  <div className="section-icon"><Volume2 size={18} /></div>
                  <div>
                    <h2 id="phonetic-search-title">音标检索</h2>
                    <p>使用音标键盘输入，或直接按键盘的旧版映射按键。</p>
                  </div>
                </div>

                <form
                  id="phonetic-search-form"
                  className="search-form"
                  onSubmit={handlePhoneticSearch}
                >
                  <label className="sr-only" htmlFor="phonetic-query">音标查询</label>
                  <div className="search-input-wrap">
                    <Volume2 className="input-sound-icon" size={19} />
                    <input
                      ref={phoneticInputRef}
                      id="phonetic-query"
                      className="phonetic-input"
                      autoComplete="off"
                      spellCheck={false}
                      value={phoneticQuery}
                      onChange={(event) => setPhoneticQuery(event.target.value)}
                      onKeyDown={handlePhysicalKey}
                      placeholder="例如：iːˌæŋwɪn"
                      aria-describedby="phonetic-input-help"
                    />
                    {phoneticQuery && (
                      <button
                        className="clear-button"
                        type="button"
                        onClick={() => {
                          setPhoneticQuery("")
                          setPhoneticState("idle")
                          setPhoneticError("")
                          phoneticInputRef.current?.focus()
                        }}
                        aria-label="清空输入"
                      >
                        清空
                      </button>
                    )}
                  </div>
                  <button className="search-button" type="submit" disabled={phoneticState === "loading"}>
                    <Search size={17} />
                    {phoneticState === "loading" ? "查询中…" : "查询音标"}
                  </button>
                </form>
                <div className="search-footnote" id="phonetic-input-help">
                  <span>支持输入标准 Unicode 国际音标或使用下方虚拟键盘输入。</span>
                  <button
                    className="keyboard-toggle"
                    type="button"
                    onClick={() => setKeyboardOpen((open) => !open)}
                    aria-expanded={keyboardOpen}
                  >
                    <Keyboard size={15} />
                    {keyboardOpen ? "收起键盘" : "显示键盘"}
                  </button>
                </div>

                {keyboardOpen && (
                  <div className="phonetic-keyboard" aria-label="虚拟音标键盘">
                    {phoneticKeyboardRows.map((row, rowIndex) => (
                      <div className="keyboard-row" key={rowIndex}>
                        {row.map((key) => (
                          <button
                            className="phonetic-key"
                            key={`${rowIndex}-${key.label}`}
                            type="button"
                            onClick={() => insertPhoneticKey(key.value)}
                            aria-label={`输入音标字符 ${key.value}`}
                          >
                            {key.label}
                          </button>
                        ))}
                      </div>
                    ))}
                    <div className="keyboard-row keyboard-actions">
                      <button
                        className="phonetic-key utility-key backspace-key"
                        type="button"
                        onClick={() => {
                          const input = phoneticInputRef.current
                          if (!input) return
                          const start = input.selectionStart ?? phoneticQuery.length
                          const end = input.selectionEnd ?? phoneticQuery.length
                          const from = start === end ? Math.max(0, start - 1) : start
                          setPhoneticQuery(`${phoneticQuery.slice(0, from)}${phoneticQuery.slice(end)}`)
                          requestAnimationFrame(() => {
                            input.focus()
                            input.setSelectionRange(from, from)
                          })
                        }}
                      >
                        退格
                      </button>
                      <button
                        className="phonetic-key enter-key"
                        type="submit"
                        form="phonetic-search-form"
                      >
                        回车查询
                      </button>
                    </div>
                  </div>
                )}
              </section>

              {phoneticError && (
                <div className="alert-error" role="alert">
                  <strong>暂时无法完成操作</strong>
                  <span>{phoneticError}</span>
                </div>
              )}

              <section className="result-card" aria-labelledby="phonetic-result-title" aria-live="polite">
                <div className="result-header">
                  <div>
                    <p className="eyebrow">DICTIONARY ENTRY</p>
                    <h2 id="phonetic-result-title">词条信息</h2>
                  </div>
                  {selectedPhonetic && phoneticExact !== null && (
                    <span className={`match-badge ${phoneticExact ? "exact" : "approximate"}`}>
                      <i />
                      {phoneticExact ? "精确匹配" : "最近匹配"}
                    </span>
                  )}
                </div>

                {selectedPhonetic ? (
                  <>
                    <div className="entry-primary">
                      <div>
                        <span className="field-label">音标</span>
                        <p className="entry-pronunciation">{selectedPhonetic.pronunciation}</p>
                      </div>
                      <span className="entry-number">
                        索引 #{selectedPhonetic.index.toLocaleString("zh-CN")}
                      </span>
                    </div>
                    <div className="entry-word">
                      <span className="field-label">词条原始字段</span>
                      <pre>{selectedPhonetic.word}</pre>
                      <p>词条字段按词典原始内容显示，字段内的制表符会保留。</p>
                    </div>
                    <div className="entry-navigation">
                      <span>
                        {selectedPhonetic.index.toLocaleString("zh-CN")} / {formatCount(phoneticCount)}
                      </span>
                      <div>
                        <button
                          type="button"
                          className="icon-control"
                          onClick={() => movePhoneticEntry(-1)}
                          disabled={selectedPhonetic.index <= 1 || phoneticState === "loading"}
                          aria-label="上一条"
                          title="上一条"
                        >
                          <ChevronLeft size={18} />
                        </button>
                        <button
                          type="button"
                          className="icon-control"
                          onClick={() => movePhoneticEntry(1)}
                          disabled={phoneticCount === null || selectedPhonetic.index >= phoneticCount || phoneticState === "loading"}
                          aria-label="下一条"
                          title="下一条"
                        >
                          <ChevronRight size={18} />
                        </button>
                      </div>
                    </div>
                  </>
                ) : (
                  <div className="empty-result">
                    <span className="empty-icon"><Volume2 size={22} /></span>
                    <strong>搜索结果将在这里显示</strong>
                    <p>输入音标并查询后，可查看对应条目和索引位置。</p>
                  </div>
                )}
              </section>
            </>
          )}

          <p className="data-note">
            查询内容会发送到已连接的 Go 词典服务；音标已转换为标准 Unicode 国际音标（IPA）渲染，无需安装专用字体即可跨平台正常显示。
          </p>
        </div>
      </main>
    </div>
  )
}
