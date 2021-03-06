#include <iostream>
#include <string>
#include <ctype.h>
#include <utility>
#include "lexicalAnalysis.h"

std::string scanner::preprocessing(std::string code)
{
	for (int i = 0; i < code.size(); i++)
	{	//delete single-line annotation.
		if (code[i] == '/' && code[i + 1] == '/')
		{
			int j = i + 2;
			while (j < code.size() && code[j] != '\n')j++;
			code.erase(i, j - i + 1);
		}
		//delete mutiple-line annotation
		if (code[i] == '/' && code[i + 1] == '*')
		{
			int j = i + 2;
			for (; j < code.size(); j++)
				if (code[j - 1] == '*' && code[j] == '/') break;
			code.erase(i, j - i + 1);
		}
		//delete carriage-return and tab
		if (code[i] == '\n' || code[i] == '\t')
		{
			code.erase(i--, 1);
		}
	}
	return code;
}

size_t scanner::isKeyWord(const std::string& word)
{
	if (this->sym_table.find(word) != this->sym_table.end() && this->sym_table.at(word) < 12)
	{
		return this->sym_table.at(word);
	}
	return 0;
}

size_t scanner::isSeparater(const std::string& word)
{
	if (this->sym_table.find(word) != this->sym_table.end() && this->sym_table.at(word) >= 12 && this->sym_table.at(word) < 22)
	{
		return this->sym_table.at(word);
	}
	return 0;
}

size_t scanner::isOperator(const std::string& word)
{
	if (this->sym_table.find(word) != this->sym_table.end() && this->sym_table.at(word) >= 22)
	{
		return this->sym_table.at(word);
	}
	return 0;
}

bool scanner::isNumber(const std::string& word)
{
	bool decimal = false;
	for (char ch : word)
	{
		if (ch == '.')
		{
			if (decimal)
			{
				return false;
			}
			else
			{
				decimal = true;
			}
		}
		else if (!isdigit(ch))
		{
			return false;
		}
	}
	return true;
}

bool scanner::isVariaty(const std::string& word)
{
	if (!isalpha(word[0]) && word[0] != '_')return "";
	for (char ch : word)
	{
		if (!iscsym(ch))
		{
			return false;
		}
	}
	return true;
}

std::list<std::pair<size_t, std::string>> scanner::analysis(const std::string& code)
{
	std::list<std::pair<size_t, std::string>> res;
	std::string word;
	//insert into the list according to the lexical type
	auto insertWord = [&]() {
		size_t id = isKeyWord(word);
		if (id)
		{
			res.push_back(std::make_pair(id, KEYWORD));
			word.clear();
		}
		else
		{
			if (isNumber(word))
			{
				res.push_back(std::make_pair(101, word));
				word.clear();
			}
			else
			{
				try
				{
					if (isVariaty(word))
					{
						res.push_back(std::make_pair(100, word));
						word.clear();
					}
					else
					{
						throw std::runtime_error(ILLEGAL_WORD);
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "ERROR:" << e.what() << std::endl;
					exit(0);
				}

			}
		}
	};
	for (char ch : code)
	{
		try
		{
			//exclude character out of ascii set
			if (!isascii(ch))throw std::runtime_error(ILLEGAL_WORD);
			//distinguish words by space
			if (ch != ' ')
			{
				std::string c;
				c.push_back(ch);
				size_t id = isSeparater(c);
				if (id)
				{
					if (word.size())
					{
						insertWord();
						word.clear();
					}
					res.push_back(std::make_pair(id, SEPARATOR));
				}
				else
				{
					id = isOperator(c);
					if (id)
					{
						if (word.size())
						{
							insertWord();
							word.clear();
						}
						res.push_back(std::make_pair(id, OPERATOR));
					}
					else
					{
						word.push_back(ch);
					}
				}
			}
			else
			{
				insertWord();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "ERROR:" << e.what() << std::endl;
			exit(0);
		}
	}
	auto iter = res.begin();
	while (iter != res.end())
	{ 
		//combine '>'&'=','<'&'=','='&'=','!'&'=',
		size_t ch1 = 0, ch2 = 0;
		ch1 = iter->first;
		++iter;
		if (iter != res.end())ch2 = iter->first;
		--iter;
		if (ch2 == 32 && (ch1 == 26 || ch1 == 27 || ch1 == 32 || ch1 == 35))
		{
			switch (ch1)
			{
			case 26:iter->first = 29;break;
			case 27:iter->first = 30;break;
			case 32:iter->first = 28;break;
			case 35:iter->first = 31;break;
			}
			iter = res.erase(++iter);
			continue;
		}
		size_t quotation = iter->first;
		//formatting const string
		if (quotation == 14 || quotation == 15)
		{ 
			std::string const_str;
			auto now = iter;
			++iter;
			while (iter != res.end() && iter->first != quotation)
			{
				const_str.append(iter->second);
				const_str.append(" ");
				++iter;
			}
			const_str.pop_back();
			now->first = 102;
			now->second = const_str;
			iter = res.erase(++now, ++iter);
		}
		//combine '&&' '||'
		if (quotation == 33 || quotation == 34)
		{
			if ((++iter)->first == quotation)
			{
				iter->first = quotation + 3;
				iter = res.erase(--iter);
			}
		}
		if (quotation == 23)
		{
			if ((--iter)->first < 100)
			{
				res.insert(++iter, std::make_pair(101, "0"));
			}
			else ++iter;
		}
		//formatting 'array[]'
		if (quotation == 18)
		{
			auto last = --iter;
			auto size = ++(++iter);
			auto end = ++(++iter);
			std::pair<size_t, std::string> arrayname = { 104,last->second + "[" + size->second + "]" };
			iter = res.erase(last, end);
			res.insert(iter, arrayname);
		}
		++iter;
	}
	return res;
}