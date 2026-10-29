import React from "react"
import { Volume2, Tag, BookA, ArrowRight } from "lucide-react"
import { parseDefinition } from "./definitionParser"

interface DefinitionViewProps {
  rawText?: string
  title: string
  onSelectPhonetic?: (phonetic: string) => void
  onSelectWord?: (word: string) => void
}

/**
 * Highlights keywords wrapped in #...# in example sentences.
 */
function renderHighlightedSentence(text: string): React.ReactNode {
  if (!text.includes("#")) {
    return text
  }
  const parts = text.split("#")
  return parts.map((part, index) => {
    // Odd index is inside #...#
    if (index % 2 === 1) {
      return (
        <strong key={index} className="example-keyword">
          {part}
        </strong>
      )
    }
    return part
  })
}

export const DefinitionView: React.FC<DefinitionViewProps> = ({
  rawText,
  title,
  onSelectPhonetic,
  onSelectWord,
}) => {
  if (!rawText || !rawText.trim()) {
    return (
      <div className="definition-card">
        <div className="definition-card-header">
          <span className="field-label">{title}</span>
        </div>
        <p className="definition-empty">暂无{title}内容</p>
      </div>
    )
  }

  const parsed = parseDefinition(rawText)

  const hasContent =
    parsed.phonetics.length > 0 ||
    parsed.inflections.length > 0 ||
    parsed.sections.length > 0 ||
    parsed.synonyms.length > 0 ||
    parsed.antonyms.length > 0 ||
    parsed.relatedWords.length > 0 ||
    parsed.fallbackText

  if (!hasContent) {
    return (
      <div className="definition-card">
        <div className="definition-card-header">
          <span className="field-label">{title}</span>
        </div>
        <p className="definition-empty">暂无{title}内容</p>
      </div>
    )
  }

  return (
    <div className="definition-card">
      <div className="definition-card-header">
        <span className="field-label">{title}</span>
      </div>

      {/* Phonetic badges */}
      {parsed.phonetics.length > 0 && (
        <div className="definition-phonetics">
          {parsed.phonetics.map((p, idx) => (
            <button
              key={idx}
              type="button"
              className="phonetic-chip"
              onClick={() => onSelectPhonetic?.(p)}
              title={`点击在音标索引中查询 [${p}]`}
            >
              <Volume2 size={13} />
              <span>[{p}]</span>
              <ArrowRight size={11} className="chip-arrow" />
            </button>
          ))}
        </div>
      )}

      {/* Inflections */}
      {parsed.inflections.length > 0 && (
        <div className="inflections-bar">
          <Tag size={12} />
          <span>{parsed.inflections.join(" · ")}</span>
        </div>
      )}

      {/* Sections by Part of Speech */}
      {parsed.sections.length > 0 ? (
        <div className="property-sections">
          {parsed.sections.map((sec, sIdx) => (
            <div key={sIdx} className="property-section">
              {sec.partOfSpeech && (
                <div className="pos-badge-wrap">
                  <span className="pos-badge">{sec.partOfSpeech}</span>
                </div>
              )}
              <div className="senses-list">
                {sec.senses.map((sense, senseIdx) => (
                  <div key={senseIdx} className="sense-item">
                    <div className="sense-header">
                      {sec.senses.length > 1 && (
                        <span className="sense-index">{senseIdx + 1}.</span>
                      )}
                      {sense.chinese && (
                        <span className="sense-chinese">{sense.chinese}</span>
                      )}
                    </div>

                    {sense.english && (
                      <p className="sense-english">{sense.english}</p>
                    )}

                    {sense.examples.length > 0 && (
                      <div className="sense-examples">
                        {sense.examples.map((ex, exIdx) => (
                          <div key={exIdx} className="example-item">
                            {ex.english && (
                              <p className="example-en">
                                {renderHighlightedSentence(ex.english)}
                              </p>
                            )}
                            {ex.chinese && (
                              <p className="example-zh">{ex.chinese}</p>
                            )}
                          </div>
                        ))}
                      </div>
                    )}
                  </div>
                ))}
              </div>
            </div>
          ))}
        </div>
      ) : parsed.fallbackText ? (
        <pre className="definition-raw-fallback">{parsed.fallbackText}</pre>
      ) : null}

      {/* Synonyms */}
      {parsed.synonyms.length > 0 && (
        <div className="word-tags-group">
          <span className="tags-label">同义词：</span>
          <div className="tags-container">
            {parsed.synonyms.map((word, wIdx) => (
              <button
                key={wIdx}
                type="button"
                className="word-link-tag"
                onClick={() => onSelectWord?.(word)}
                title={`查询单词 ${word}`}
              >
                {word}
              </button>
            ))}
          </div>
        </div>
      )}

      {/* Antonyms */}
      {parsed.antonyms.length > 0 && (
        <div className="word-tags-group">
          <span className="tags-label">反义词：</span>
          <div className="tags-container">
            {parsed.antonyms.map((word, wIdx) => (
              <button
                key={wIdx}
                type="button"
                className="word-link-tag"
                onClick={() => onSelectWord?.(word)}
                title={`查询单词 ${word}`}
              >
                {word}
              </button>
            ))}
          </div>
        </div>
      )}

      {/* Related words */}
      {parsed.relatedWords.length > 0 && (
        <div className="word-tags-group">
          <span className="tags-label">
            <BookA size={12} /> 相关词：
          </span>
          <div className="tags-container">
            {parsed.relatedWords.map((word, wIdx) => (
              <button
                key={wIdx}
                type="button"
                className="word-link-tag"
                onClick={() => onSelectWord?.(word)}
                title={`查询单词 ${word}`}
              >
                {word}
              </button>
            ))}
          </div>
        </div>
      )}
    </div>
  )
}
