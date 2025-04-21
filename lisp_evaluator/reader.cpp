#include "reader.h"

reader_t::reader_t(istream& is, function<expr_t*(const string&)> default_lexeme_handler):
  is(is),
  skippable_predicate([](char c) { return false; }),
  default_lexeme_handler(default_lexeme_handler)
{
  terminators.push([](reader_t& reader) {
    return reader.is_at_end();
  });
}

void reader_t::set_skippable_predicate(function<bool(char)> skippable_predicate) {
  this->skippable_predicate = skippable_predicate;
}

void reader_t::add(const string& lexeme, function<expr_t*(reader_t&)> f) {
  size_t index_lexeme = 0;
  node_t* node = &root;
  while (index_lexeme < lexeme.size()) {
    char c = lexeme[index_lexeme++];
    if (!node->children[c]) {
      node->children[c] = new node_t();
    }
    node = node->children[c];
  }
  node->f = f;
}

expr_t* reader_t::read(function<bool(reader_t&)> terminator) {
  terminators.push(terminator);
  expr_t* result = read(is);
  terminators.pop(terminator);
  return result;
}

expr_t* reader_t::read() {
  while (!is_at_end() && skippable_predicate(peek())) {
    eat(is);
  }
  
  m_lexeme.clear();
  node_t* node = &root;

  function<expr_t*(reader_t&)> f;
  while (!is_at_end()) {
    node = node->children[peek()];
    if (node) {
      f = node->f;
      eat();
    } else {
      break ;
    }
  }

  if (f) {
    cout << "READER MACRO FOUND: '" << m_lexeme << "'" << endl;
    m_lexeme.clear();
    return f(*this);
  } else { 
    function<bool(istream&)> terminator;
    terminator = terminators.top();
    while (!terminator(is)) {
      eat();
    }
    if (m_lexeme.empty()) {
      return 0;
    }
    cout << "READER MACRO NOT FOUND, LEXEME: '" << m_lexeme << "'" << endl;
    return default_lexeme_handler(m_lexeme);
  }
}

char reader_t::peek() const {
  return is.peek();
}

char reader_t::eat() {
  char result = is.get();
  m_lexeme.push_back(result);
  return result;
}

bool reader_t::is_at_end() const {
  return is && is.peek() == EOF;
}

bool reader_t::is_whitespace(char c) const {
  return skippable_predicate(char c);
}

const string reader_t::lexeme() const {
  return m_lexeme;
}

reader_t::node_t::node_t() {
  for (int i = 0; i < sizeof(children) / sizeof(children[0]); ++i) {
    children[i] = 0;
  }
}
