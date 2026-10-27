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
import { getEntry, getEntryCount, searchPhonetic } from "./api"
import type { PhoneticEntry } from "./api"
import { mapPhysicalKey, phoneticKeyboardRows } from "./phoneticKeys"

type SearchState = "idle" | "loading" | "error" | "ready"

function formatCount(value: number | null) {
  return value === null ? "加载中…" : new Intl.NumberFormat("zh-CN").format(value)
}

export default function App() {
  const [query, setQuery] = useState("")
  const [entryCount, setEntryCount] = useState<number | null>(null)
  const [selectedEntry, setSelectedEntry] = useState<PhoneticEntry | null>(null)
  const [exactMatch, setExactMatch] = useState<boolean | null>(null)
  const [searchState, setSearchState] = useState<SearchState>("idle")
  const [errorMessage, setErrorMessage] = useState("")
  const [keyboardOpen, setKeyboardOpen] = useState(true)
  const inputRef = useRef<HTMLInputElement>(null)
  const requestRef = useRef<AbortController | null>(null)

  useEffect(() => {
    const controller = new AbortController()
    getEntryCount(controller.signal)
      .then(setEntryCount)
      .catch((error: unknown) => {
        if (error instanceof DOMException && error.name === "AbortError") return
        setErrorMessage(error instanceof Error ? error.message : "读取词典信息失败。")
      })
    return () => controller.abort()
  }, [])

  useEffect(() => () => requestRef.current?.abort(), [])

  function insertKey(value: string) {
    const input = inputRef.current
    if (!input) return
    const start = input.selectionStart ?? query.length
    const end = input.selectionEnd ?? query.length
    const nextQuery = `${query.slice(0, start)}${value}${query.slice(end)}`
    setQuery(nextQuery)
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
    if (mapped) insertKey(mapped)
  }

  async function handleSearch(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    const value = query.trim()
    if (!value) {
      setErrorMessage("请输入或选择要查询的音标。")
      setSearchState("error")
      inputRef.current?.focus()
      return
    }

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setSearchState("loading")
    setErrorMessage("")
    try {
      const result = await searchPhonetic(value, controller.signal)
      setSelectedEntry({
        index: result.index,
        pronunciation: result.pronunciation,
        word: result.word,
      })
      setExactMatch(result.exact)
      setSearchState("ready")
    } catch (error) {
      if (error instanceof DOMException && error.name === "AbortError") return
      setErrorMessage(error instanceof Error ? error.message : "音标搜索失败。")
      setSearchState("error")
    }
  }

  async function moveEntry(offset: -1 | 1) {
    if (!selectedEntry || !entryCount) return
    const nextIndex = selectedEntry.index + offset
    if (nextIndex < 1 || nextIndex > entryCount) return

    requestRef.current?.abort()
    const controller = new AbortController()
    requestRef.current = controller
    setSearchState("loading")
    setErrorMessage("")
    try {
      const entry = await getEntry(nextIndex, controller.signal)
      setSelectedEntry(entry)
      setExactMatch(null)
      setQuery(entry.pronunciation)
      setSearchState("ready")
      inputRef.current?.focus()
    } catch (error) {
      if (error instanceof DOMException && error.name === "AbortError") return
      setErrorMessage(error instanceof Error ? error.message : "读取音标词条失败。")
      setSearchState("error")
    }
  }

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
          <a className="nav-link active" href="#phonetic-search">
            <Volume2 size={17} />
            <span>音标索引</span>
            <ChevronRight className="nav-chevron" size={15} />
          </a>
          <span className="nav-link disabled" aria-disabled="true">
            <BookOpen size={17} />
            <span>单词查询</span>
            <small>即将推出</small>
          </span>
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

      <main className="main-area" id="phonetic-search">
        <header className="topbar">
          <div className="breadcrumb">
            <span>词典工作台</span>
            <ChevronRight size={14} />
            <strong>音标索引</strong>
          </div>
          <span className="service-indicator">
            <i className={entryCount === null ? "connecting" : ""} />
            {entryCount === null ? "连接服务中" : "词典服务已连接"}
          </span>
        </header>

        <div className="page-content">
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
              <strong>{formatCount(entryCount)}</strong>
            </div>
          </section>

          <section className="workspace-card" aria-labelledby="search-title">
            <div className="section-heading">
              <div className="section-icon"><Search size={18} /></div>
              <div>
                <h2 id="search-title">音标检索</h2>
                <p>使用音标键盘输入，或直接按键盘的旧版映射按键。</p>
              </div>
            </div>

            <form
              id="phonetic-search-form"
              className="search-form"
              onSubmit={handleSearch}
            >
              <label className="sr-only" htmlFor="phonetic-query">音标查询</label>
              <div className="search-input-wrap">
                <Volume2 className="input-sound-icon" size={19} />
                <input
                  ref={inputRef}
                  id="phonetic-query"
                  className="phonetic-input"
                  autoComplete="off"
                  spellCheck={false}
                  value={query}
                  onChange={(event) => setQuery(event.target.value)}
                  onKeyDown={handlePhysicalKey}
                  placeholder="例如：i:5AnWin"
                  aria-describedby="input-help"
                />
                {query && (
                  <button
                    className="clear-button"
                    type="button"
                    onClick={() => {
                      setQuery("")
                      setSearchState("idle")
                      setErrorMessage("")
                      inputRef.current?.focus()
                    }}
                    aria-label="清空输入"
                  >
                    清空
                  </button>
                )}
              </div>
              <button className="search-button" type="submit" disabled={searchState === "loading"}>
                <Search size={17} />
                {searchState === "loading" ? "查询中…" : "查询音标"}
              </button>
            </form>
            <div className="search-footnote" id="input-help">
              <span>音标输入使用旧版内部编码，不支持直接输入 Unicode IPA。</span>
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
                        onClick={() => insertKey(key.value)}
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
                      const input = inputRef.current
                      if (!input) return
                      const start = input.selectionStart ?? query.length
                      const end = input.selectionEnd ?? query.length
                      const from = start === end ? Math.max(0, start - 1) : start
                      setQuery(`${query.slice(0, from)}${query.slice(end)}`)
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

          {errorMessage && (
            <div className="alert-error" role="alert">
              <strong>暂时无法完成操作</strong>
              <span>{errorMessage}</span>
            </div>
          )}

          <section className="result-card" aria-labelledby="result-title" aria-live="polite">
            <div className="result-header">
              <div>
                <p className="eyebrow">DICTIONARY ENTRY</p>
                <h2 id="result-title">词条信息</h2>
              </div>
              {selectedEntry && exactMatch !== null && (
                <span className={`match-badge ${exactMatch ? "exact" : "approximate"}`}>
                  <i />
                  {exactMatch ? "精确匹配" : "最近匹配"}
                </span>
              )}
            </div>
            {selectedEntry ? (
              <>
                <div className="entry-primary">
                  <div>
                    <span className="field-label">音标</span>
                    <p className="entry-pronunciation">{selectedEntry.pronunciation}</p>
                  </div>
                  <span className="entry-number">索引 #{selectedEntry.index.toLocaleString("zh-CN")}</span>
                </div>
                <div className="entry-word">
                  <span className="field-label">词条原始字段</span>
                  <pre>{selectedEntry.word}</pre>
                  <p>词条字段按词典原始内容显示，字段内的制表符会保留。</p>
                </div>
                <div className="entry-navigation">
                  <span>
                    {selectedEntry.index.toLocaleString("zh-CN")} / {formatCount(entryCount)}
                  </span>
                  <div>
                    <button
                      type="button"
                      className="icon-control"
                      onClick={() => moveEntry(-1)}
                      disabled={selectedEntry.index <= 1 || searchState === "loading"}
                      aria-label="上一条"
                    >
                      <ChevronLeft size={18} />
                    </button>
                    <button
                      type="button"
                      className="icon-control"
                      onClick={() => moveEntry(1)}
                      disabled={entryCount === null || selectedEntry.index >= entryCount || searchState === "loading"}
                      aria-label="下一条"
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

          <p className="data-note">
            查询内容会发送到已连接的 Go 词典服务；音标字形需要本机安装
            “Okeyen Phonetic” 字体，未安装时使用等宽字体显示内部编码。
          </p>
        </div>
      </main>
    </div>
  )
}
