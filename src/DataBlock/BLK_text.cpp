#include "DataBlock.h"
#include "Logger.h"

bool isSpacingCharacter(char chr) {
  return chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t';
}

struct TextStream {
  std::span<char> *text;
  size_t offset = 0;

  bool is_EOF()
  {
    return offset >= text->size();
  }

  char getCurrent() {
    return (*text)[offset-1];
  }

  inline char getNext(bool &fail, bool do_warn = true) {
    if (is_EOF()) {
      if(do_warn)
        LOG("reached EOF before finished parsing BLK");
      fail = true;
      return 0;
    }
    return (*text)[offset++];
  }

  char getNextCharacter(bool &fail, bool do_warn = true) {
    char currChar = getNext(fail, do_warn);
    if(!isSpacingCharacter(currChar))
      return currChar;
    if (fail)
      return 0;
    for (; isSpacingCharacter(currChar); currChar = getNext(fail, do_warn)) {
      if (fail)
        return 0;
    }
    return currChar;
  }

  /// reads in a string, stops when it reaches an end char
  /// \param out the string where the read data will be written to
  /// \return returns if the read was successful
  bool readString(std::string &out)
  {
    bool fail = false;
    for (char c = getNextCharacter(fail); !isSpacingCharacter(c); c = getNext(fail)) {
      if (fail)
        return false;
      out.push_back(c);
    }
    return true;
  }

  /// reads a string, ending when has reached a specified endChar
  /// \param out the string where the read data will be written to
  /// \param endChar the end char to match
  /// \param spacing_allowed if spaces are allowed in the string
  /// \return returns if the read was successful
  bool readString(std::string &out, char endChar, char spacing_allowed)
  {
    bool fail = false;
    for (char c = getNextCharacter(fail); c != endChar; c = getNext(fail)) {
      if (fail)
        return false;
      if(!spacing_allowed)
        if (isSpacingCharacter(c)) {
          LOG("invalid string, space or newline found in variable name and spacing was not allowed");
          return false;
        }
      out.push_back(c);
    }
    return true;
  }

  bool ReadUntilNum()
  {
    bool fail=false;
    for (char c = getNextCharacter(fail); c < '0' || c > '9'; c = getNext(fail)) {
      if(fail)
        return false;
    }
    if(fail)
      return false;
    this->goBack();
    return true;
  }

  bool reverseTillChar(char endChar) {
    //bool fail = false;
    for(char c = getCurrent(); c != endChar; c = getCurrent(), this->goBack())
    {}
    return true;
  }

  void goBack() {
    --offset;
  }

};

bool readObjName(std::string &out, TextStream &strm) {
  bool fail = false;
  for (char c = strm.getNextCharacter(fail); c != '=' && c != '{'; c = strm.getNext(fail)) {
    if (fail)
      return false;
    if (isSpacingCharacter(c)) {
      LOG("invalid text blk, space or newline found in variable name");
      return false;
    }

    out.push_back(c);
  }
  if (fail)
    return false;
  return true;
}

#define retFail(succeed) if(!succeed) return false
uint64_t readInt();


bool ReadParam(std::string &name, DataBlock *blk, TextStream &strm)
{
  std::string param_type;
  std::string data;
  retFail(strm.readString(param_type, '=', false));
  //if(!strm.readString(data)) return false;

  if (param_type.size() == 1)
  {
    switch(*param_type.data())
    {
      case 's':
      case 't': // text
      {
        bool fail = false;
        char t = strm.getNext(fail);
        if(fail)
          return false;
        if(t != '\"')
        {
          LOG("Expected \" at start of string while parsing param {}", name);
          return false;
        }
        retFail(strm.readString(data, '\"', true));
        blk->addStr(name, data);
        return true;
      }
      case 'b': // bool
      {
        retFail(strm.readString(data));
        bool payload;
        if(data == "yes")
          payload = true;
        else if(data == "no")
          payload = false;
        else {
          LOG("Invalid bool value '{}' while parsing param {}", data, name);
          return false;
        }
        blk->addBool(name, payload);
        return true;
      }
      case 'i': // integer
      {
        retFail(strm.readString(data));
        int v = std::stoi(data); // throws exception on too large
        blk->addInt(name, v);
        return true;
      }
      case 'r': // real / float
      {
        retFail(strm.readString(data));
        float f = std::stof(data);
        blk->addReal(name, f);
        return true;
      }
      case 'c': // E3D Color
      {
        break;
      }
    }
  }
  if (param_type == "ip3")
  {
    retFail(strm.ReadUntilNum());
    IPoint3 payload;
    std::string str{};
    int v;
    retFail(strm.readString(str, ',', false));
    v = std::stoi(str); // throws exception on too large
    payload.x = v;
    str.clear();
    retFail(strm.ReadUntilNum());
    retFail(strm.readString(str, ',', false));
    v = std::stoi(str); // throws exception on too large
    payload.y = v;
    str.clear();
    retFail(strm.ReadUntilNum());
    retFail(strm.readString(str, '\n', true));
    v = std::stoi(str); // throws exception on too large
    payload.z = v;
    blk->addIPoint3(name, payload);
    return true;
  }
  else if(param_type == "p2") {
    retFail(strm.ReadUntilNum());
    Point2 payload;
    std::string str{};
    retFail(strm.readString(str, ',', false));
    float v = std::stof(str);
    payload.x = v;
    str.clear();
    retFail(strm.ReadUntilNum());
    retFail(strm.readString(str, '\n', true));
    v = std::stof(str); // throws exception on too large
    payload.y = v;
    str.clear();
    blk->addPoint2(name, payload);
    return true;
  }
  LOG("failed to parse type '{}' for param {}", param_type, name);
  return false;
}

bool LoadBlock(DataBlock *blk, TextStream &strm, int stack = 0) {
  while (!strm.is_EOF()) {
    bool ret = false;
    char first = strm.getNextCharacter(ret, false); // the EOF can be expected here
    if(first == '}') // end of block
      return true;
    else
      strm.goBack();
    if(ret) // eof
      break;

    std::string name{};
    retFail(readObjName(name, strm));
    switch (strm.getCurrent()) {
      case '{': {
        auto new_blk = blk->getBlock(blk->addBlock(name));
        retFail(LoadBlock(new_blk.get(), strm, stack+1));
        break;
      }
      case '=': {
        size_t start_pos = strm.offset;
        strm.reverseTillChar(':');
        name.resize(name.size()-(start_pos-(strm.offset+1)));

        strm.offset++;
        retFail(ReadParam(name, blk, strm));
        break;
      }
      default: {
        LOG("Invalid token after object name, expected {{ or :");
        return false;
      }
    }
  }
  if(stack == 0) // top level block
    return true;
  return false;
}


bool DataBlock::loadText(std::span<char> &text) {
  if (text.empty())
    return true;
  auto strm = TextStream{&text, 0};
  return ::LoadBlock(this, strm);
}
