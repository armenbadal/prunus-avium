local Lexer = {}
Lexer.__index = Lexer

local Token = {
    Terminal = 1,
    Nonterminal = 2,
    Metasymbol = 3,
    NewLine = 4,
    Space = 5
}

function Lexer.new(input)
    return setmetatable({
        input = input,
        pos = 1,
        len = #input
    }, Lexer)
end

function Lexer:peek()
    return self.input:sub(self.pos, self.pos)
end

function Lexer:advance()
    self.pos = self.pos + 1
end

function Lexer:match(pattern)
    local s, e = self.input:find(pattern, self.pos)
    if s == self.pos then
        local text = self.input:sub(s, e)
        self.pos = e + 1
        return text
    end
end

function Lexer:next_token()
    if self.pos > self.len then
        return nil
    end

    local ch = self:peek()

    if ch == "\n" then
        self:advance()
        return { type = Token.NewLine, value = "\\\\", length = 1 }
    end

    if ch == " " or ch == "\t" then
        self:advance()
        return { type = Token.Space, value = "~", length = 1 }
    end

    if string.find("=(){}[].|", ch, 1, true) then
        self:advance()
        if ch == "{" or ch == "}" then
            ch = "\\" .. ch
        elseif ch == "=" then
            ch = "&=&"
        end
        return { type = Token.Metasymbol, value = ch, length = 1 }
    end

    if ch == "'" then
        self:advance()
        local start = self.pos

        while self.pos <= self.len and self:peek() ~= "'" do
            self:advance()
        end

        if self.pos > self.len then
            error("Unterminated quoted string")
        end

        local content = self.input:sub(start, self.pos - 1)
        self:advance()

        return { type = Token.Terminal, value = "`\\Verb|" .. content .. "|'", length = #content }
    end

    local name = self:match("^[A-Z][a-zA-Z]*")
    if name:match("^[A-Z]+$") then
        return { type = Token.Terminal, value = "\\texttt{" .. name .. "}", length = #name }
    else
        return { type = Token.Nonterminal, value = "\\textit{" .. name .. "}", length = #name }
    end

    error("Unexpected character at position " .. self.pos)
end

function Lexer:tokens()
    return function()
        return self:next_token()
    end
end


local function is_first(line)
    if #line < 2 then
        return false
    end

    local ix = 1
    while line[ix].type == Token.Space do
        ix = ix + 1
    end
    if line[ix].type ~= Token.Nonterminal then
        return false
    end

    ix = ix + 1
    while line[ix].type == Token.Space do
        ix = ix + 1
    end
    return line[ix].type == Token.Metasymbol and line[ix].value == "&=&"
end

local function is_alternative(line)
    local ix = 1
    while line[ix].type == Token.Space do
        ix = ix + 1
    end
    return line[ix].type == Token.Metasymbol and line[ix].value == "|"
end

local function debug_print(elements)
    local text = "DEBUG: "
    for _, elem in ipairs(elements) do
        text = text .. elem.value .. " "
    end
    print(text)
end

local pos = 0
local function join(line)
    local text = ""

    if is_first(line) then
        pos = 0
        for _, elem in ipairs(line) do
            pos = pos + elem.length
            if elem.type == Token.Metasymbol and elem.value == "&=&" then
                break
            end
        end
        while #line ~= 0 and line[1].type == Token.Space do
            table.remove(line, 1)
        end
    elseif is_alternative(line) then
        while #line ~= 0 and line[1].type == Token.Space do
            table.remove(line, 1)
        end
        text = "&|& "
        table.remove(line, 1)
    else
        text = "& &"
        c = pos
        while #line ~= 0 and line[1].type == Token.Space and c > 0 do
            table.remove(line, 1)
            c = c - 1
        end
    end

    for _, elem in ipairs(line) do
        text = text .. elem.value
    end

    return text
end


local function reformat(text)
    local result = ""
    local lexer = Lexer.new(text)
    local line = {}
    for t in lexer:tokens() do
        table.insert(line, t)
        if t.type == Token.NewLine then
            result = result .. join(line) .. "\n"
            line = {}
        end
    end
    return result
end


function CodeBlock(el)
  if el.classes:includes("ebnf") then
    return pandoc.RawBlock(
      "latex",
      "\\begin{ebnf}\n" .. reformat(el.text .. "\n") .. "\\end{ebnf}"
    )
  end
end
