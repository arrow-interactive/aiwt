#include<cstdio>
#include<fstream>
#include<string>
#include<vector>
#include<unordered_map>

void replace_all(std::string& data, std::string from, std::string to)
{
    size_t pos = data.find(from);
    while(pos != std::string::npos)
    {
        data.replace(pos, from.size(), to);
        pos = data.find(from, pos + to.size());
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2)
        fprintf(stderr, "Usage: `%s srcfile`", argv[0]);

    std::ifstream src_file(argv[1]);
    std::vector<std::string> lines{};

    {
        std::string line;
        while(std::getline(src_file, line))
            lines.push_back(line);
    }

    std::unordered_map<std::string, std::string[2]> references{};

    for(size_t li = 0; li < lines.size(); li++)
    {
        std::string line = lines[li];

        for(size_t i = 0; i < line.length(); i++)
        {
            char ch = line[i];

            if(ch == ' ' || ch == '\t')
                continue;

            if(ch != '[')
                break;

            std::string refid, href, title;
            i++;

            {
                size_t pos = line.find("]:");
                if(pos == std::string::npos)
                    break;

                refid = line.substr(i, pos - i);

                i = pos + 2;
            }

            {
                size_t substr_start = line[i] == ' ' ? i + 1 : i;
                size_t pos = line.find(' ', substr_start);
                if(pos == std::string::npos || line.find('\t', substr_start) < pos)
                    pos = line.find('\t', substr_start);

                if(pos == std::string::npos)
                {
                    href = line.substr(substr_start);
                    lines.erase(lines.begin() + li);
                    li--;
                    references[refid][0] = href;
                    references[refid][1] = title;
                    break;
                }
                else
                {
                    href = line.substr(substr_start, pos - substr_start);
                    i = pos;
                }
            }

            for(; i < line.length(); i++)
            {
                if(line[i] == ' ' || line[i] == '\t')
                    continue;

                break;
            }

            title = line.substr(i);
            lines.erase(lines.begin() + li);
            li--;
            references[refid][0] = href;
            references[refid][1] = title;
            break;
        }
    } 

    std::string out = "<!DOCTYPE html>\n<html>\n\t";

    std::vector<std::string> element_stack{};

    enum ListType
    {
        NO_LIST,
        ORDERED_LIST,
        UNORDERED_LIST,
    } list_type = NO_LIST;

    bool quote_block = false;
    bool is_quote_real = false;
    for(size_t li = 0; li < lines.size(); li++)
    {
        std::string line = lines[li];

        int spaces = 0;
        bool code_block = false;
        for(size_t i = 0; i < line.length(); i++)
        {
            char ch = line[i];

            if(ch == ' ')
            {
                spaces++;

                if(spaces >= 4)
                    code_block = true;

                continue;
            }
            if(ch == '\t')
            {
                code_block = true;
                continue;
            }

            if(isdigit(ch))
            {
                while(isdigit(line[++i]))
                    continue;

                if(line[i] != '.' && line[i] != ')')
                {
                    if(list_type == ORDERED_LIST)
                    {
                        out += "</ol>\n";
                        for(size_t j = 0; j < element_stack.size() + 1; j++)
                            out += '\t';

                        list_type = NO_LIST;
                    }

                    li--;
                    break;
                }

                if(list_type == UNORDERED_LIST)
                {
                    out += "</ul>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                    out += "<ol>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                }
                else if(list_type == NO_LIST)
                {
                    out += "<ol>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                }

                list_type = ORDERED_LIST;

                i++;

                std::unordered_map<std::string, std::string> attrs{};
                if(line[i] == '<')
                {
                    for(i++; i < line.length(); i++)
                    {
                        if(line[i] == '>')
                            break;

                        if(ch == ' ' || ch == '\t')
                            continue;

                        std::string key, value;
                        int start_index = i;
                        for(; i < line.length(); i++)
                        {
                            if(line[i] == '=')
                                break;
                        }

                        key = line.substr(start_index, i - start_index);

                        start_index = ++i;
                        for(i++; i < line.length(); i++)
                        {
                            if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                break;
                        }

                        value = line.substr(start_index, i - start_index);

                        attrs.insert(std::pair<std::string, std::string>(key, value));

                        if(line[i] == '>')
                            break;
                    }

                    i++;
                }

				size_t substr_start = line[i] == ' ' ? i + 1 : i;

                {
                    size_t start_pos = i = line.find('!', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string alt, src, title;
                        if(line[++i] != '[')
                            continue;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            alt = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('!', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                src = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, bound - i);
                            }
                            else
                            {
                                pos = bound;
                                src = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> img_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                img_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<img src=\"" + src + "\" alt=\"" + alt + "\"";
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : img_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>';

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('!');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, href, title;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                href = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, pos - i);
                            }
                            else
                            {
                                pos = bound;
                                href = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<a href=\"" + href + '"';
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = i = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, refid;

                        {
                            size_t pos = line.find(']', start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '[')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        {
                            size_t pos = line.find(']', i);
                            if(pos == std::string::npos)
                                break;

                            refid = line.substr(++i, pos - i - 1);

                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        if(references.count(refid) == 0)
                            continue;

                        std::string tmp = "<a href=\"" + references[refid][0] + '"';
                        if(references[refid][1].compare("") != 0)
                            tmp += " title=" + references[refid][1];
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t op_pos = line.find("***", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 3] == ' ')
                        {
                            op_pos = line.find("***", op_pos + 3);
                            continue;
                        }

                        cl_pos = line.find("***", op_pos + 3);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 3)
                                break;

                            cl_pos = line.find("***", cl_pos + 3);
                        }

                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 3, "<strong><em>");
                        cl_pos += 9;
                        line.replace(cl_pos, 3, "</em></strong>");

                        op_pos = line.find("***", cl_pos + 14);
                    }
                }

                {
                    size_t op_pos = line.find("**", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 2] == ' ')
                        {
                            op_pos = line.find("**", op_pos + 2);
                            continue;
                        }

                        cl_pos = line.find("**", op_pos + 2);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 2)
                                break;

                            cl_pos = line.find("**", cl_pos + 2);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 2, "<strong>");
                        cl_pos += 6;
                        line.replace(cl_pos, 2, "</strong>");

                        op_pos = line.find("**", cl_pos + 9);
                    }
                }

                {
                    size_t op_pos = line.find("*", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("*", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("*", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("*", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 1, "<em>");
                        cl_pos += 3;
                        line.replace(cl_pos, 1, "</em>");

                        op_pos = line.find("*", cl_pos + 5);
                    }
                }

                {
                    size_t op_pos = line.find("`", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("`", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("`", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("`", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        std::string tmp = line.substr(op_pos, op_pos - cl_pos);
                        replace_all(tmp, "&", "&amp;");                                     // TODO: Make inline code more conformant to markdown's standard
                        replace_all(tmp, "<", "&lt;");
                        replace_all(tmp, ">", "&gt;");
                        line.replace(op_pos, op_pos - cl_pos, tmp);

                        op_pos = line.find("`", op_pos + tmp.length());
                    }
                }

                std::string tmp = "<li";
                for(auto attr : attrs)
                    tmp += ' ' + attr.first + '=' + attr.second;
                tmp += '>' + line.substr(i) + "</li>\n";

                if(code_block)
                    out += "<code>" + tmp + "</code>\n";
                else
                    out += tmp + '\n';

                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                break;
            }
            else
            {
                if(list_type == ORDERED_LIST)
                {
                    out += "</ol>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';

                    list_type = NO_LIST;
                }
            }

            if(ch == '*' || ch == '-' || ch == '+')
            {
                if(ch == '*' || ch == '-')
                {
                    uint count = 1;

                    for(; i < line.length(); i++)
                    {
                        if(line[i] == ch)
                        {
                            count++;
                            continue;
                        }
                        else if(line[i] == ' ')
                        {
                            continue;
                        }

                        count = 0;
                        break;
                    }          

                    if(count >= 3)
                    {
                        if(list_type == UNORDERED_LIST)
                        {
                            out += "</ul>\n";
                            for(size_t j = 0; j < element_stack.size() + 1; j++)
                                out += '\t';
        
                            list_type = NO_LIST;
                        }

                        if(code_block)
                            out += "<p>&lt;hr&gt;</p>\n";
                        else
                            out += "<hr>\n";

                        for(size_t j = 0; j < element_stack.size() + 1; j++)
                            out += '\t';

                        break;
                    }
                }
                
                if(list_type == ORDERED_LIST)
                {
                    out += "</ol>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                    out += "<ul>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                }
                else if(list_type == NO_LIST)
                {
                    out += "<ul>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                }

                list_type = UNORDERED_LIST;

                i = line.find('+');
                if(line.find('-') != std::string::npos && line.find('-') < i)
                    i = line.find('-');
                if(line.find('*') != std::string::npos && line.find('*') < i)
                    i = line.find('*');

                i++;

                std::unordered_map<std::string, std::string> attrs{};
                if(line[i] == '<')
                {
                    for(i++; i < line.length(); i++)
                    {
                        if(line[i] == '>')
                            break;

                        if(ch == ' ' || ch == '\t')
                            continue;

                        std::string key, value;
                        int start_index = i;
                        for(; i < line.length(); i++)
                        {
                            if(line[i] == '=')
                                break;
                        }

                        key = line.substr(start_index, i - start_index);

                        start_index = ++i;
                        for(i++; i < line.length(); i++)
                        {
                            if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                break;
                        }

                        value = line.substr(start_index, i - start_index);

                        attrs.insert(std::pair<std::string, std::string>(key, value));

                        if(line[i] == '>')
                            break;
                    }

                    i++;
                }

				size_t substr_start = line[i] == ' ' ? i + 1 : i;

                {
                    size_t start_pos = i = line.find('!', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string alt, src, title;
                        if(line[++i] != '[')
                            continue;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            alt = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('!', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                src = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, bound - i);
                            }
                            else
                            {
                                pos = bound;
                                src = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> img_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                img_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<img src=\"" + src + "\" alt=\"" + alt + "\"";
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : img_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>';

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('!');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, href, title;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                href = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, pos - i);
                            }
                            else
                            {
                                pos = bound;
                                href = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<a href=\"" + href + '"';
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = i = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, refid;

                        {
                            size_t pos = line.find(']', start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '[')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        {
                            size_t pos = line.find(']', i);
                            if(pos == std::string::npos)
                                break;

                            refid = line.substr(++i, pos - i - 1);

                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        if(references.count(refid) == 0)
                            continue;

                        std::string tmp = "<a href=\"" + references[refid][0] + '"';
                        if(references[refid][1].compare("") != 0)
                            tmp += " title=" + references[refid][1];
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t op_pos = line.find("***", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 3] == ' ')
                        {
                            op_pos = line.find("***", op_pos + 3);
                            continue;
                        }

                        cl_pos = line.find("***", op_pos + 3);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 3)
                                break;

                            cl_pos = line.find("***", cl_pos + 3);
                        }

                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 3, "<strong><em>");
                        cl_pos += 9;
                        line.replace(cl_pos, 3, "</em></strong>");

                        op_pos = line.find("***", cl_pos + 14);
                    }
                }

                {
                    size_t op_pos = line.find("**", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 2] == ' ')
                        {
                            op_pos = line.find("**", op_pos + 2);
                            continue;
                        }

                        cl_pos = line.find("**", op_pos + 2);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 2)
                                break;

                            cl_pos = line.find("**", cl_pos + 2);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 2, "<strong>");
                        cl_pos += 6;
                        line.replace(cl_pos, 2, "</strong>");

                        op_pos = line.find("**", cl_pos + 9);
                    }
                }

                {
                    size_t op_pos = line.find("*", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("*", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("*", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("*", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 1, "<em>");
                        cl_pos += 3;
                        line.replace(cl_pos, 1, "</em>");

                        op_pos = line.find("*", cl_pos + 5);
                    }
                }

                {
                    size_t op_pos = line.find("`", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("`", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("`", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("`", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        std::string tmp = line.substr(op_pos, op_pos - cl_pos);
                        replace_all(tmp, "&", "&amp;");                                     // TODO: Make inline code more conformant to markdown's standard
                        replace_all(tmp, "<", "&lt;");
                        replace_all(tmp, ">", "&gt;");
                        line.replace(op_pos, op_pos - cl_pos, tmp);

                        op_pos = line.find("`", op_pos + tmp.length());
                    }
                }

                std::string tmp = "<li";
                for(auto attr : attrs)
                    tmp += ' ' + attr.first + '=' + attr.second;
                tmp += '>' + line.substr(i) + "</li>";

                if(code_block)
                    out += "<code>" + tmp + "</code>\n";
                else
                    out += tmp + '\n';

                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                break;
            }
            else
            {
                if(list_type == UNORDERED_LIST)
                {
                    out += "</ul>\n";
                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';

                    list_type = NO_LIST;
                }
            }
            
            if(ch == '>')
            {
                if(!quote_block)
                {
                    is_quote_real = true;

                    quote_block = true;

                    std::string tmp = "<blockquotes>";
                    if(code_block)
                    {
                        is_quote_real = false;

                        tmp = "<code>" + tmp + '\n';
                    }
                    else
                    {
                        tmp += '\n';
                    }

                    out += tmp;

                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';
                }

                spaces = 0;

                if(i >= line.length())
                    continue;

                ch = line[++i];
                while(ch != ' ' && ch != '\t' && i < line.length())
                {
                    if(ch == ' ')
                    {
                        spaces++;

                        if(spaces >= 4)
                            code_block = true;

                        continue;
                    }
                    if(ch == '\t')
                    {
                        code_block = true;
                        continue;
                    }

                    ch = line[++i];
                }
            }
            else if(ch != '>' || code_block)
            {
                if(quote_block)
                {
                    std::string tmp = "</blockquotes>";
                    if(!is_quote_real)
                    {
                        tmp += "</code>\n";
                    }
                    else
                    {
                        tmp += '\n';
                    }

                    out += tmp;

                    for(size_t j = 0; j < element_stack.size() + 1; j++)
                        out += '\t';

                    quote_block = false;
                }
            }

            if(ch == '#')
            {
                int heading_level = 1;
                while(line[++i] == '#')
                {
                    heading_level++;
                    if(heading_level == 6)
                        break;
                }

                std::unordered_map<std::string, std::string> attrs{};
                if(line[i] == '<')
                {
                    for(i++; i < line.length(); i++)
                    {
                        if(line[i] == '>')
                            break;

                        if(ch == ' ' || ch == '\t')
                            continue;

                        std::string key, value;
                        int start_index = i;
                        for(; i < line.length(); i++)
                        {
                            if(line[i] == '=')
                                break;
                        }

                        key = line.substr(start_index, i - start_index);

                        start_index = ++i;
                        for(i++; i < line.length(); i++)
                        {
                            if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                break;
                        }

                        value = line.substr(start_index, i - start_index);

                        attrs.insert(std::pair<std::string, std::string>(key, value));

                        if(line[i] == '>')
                            break;
                    }

                    i++;
                }

                size_t substr_start = line[i] == ' ' ? i + 1 : i;

                {
                    size_t start_pos = i = line.find('!', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string alt, src, title;
                        if(line[++i] != '[')
                            continue;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            alt = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('!', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                src = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, bound - i);
                            }
                            else
                            {
                                pos = bound;
                                src = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> img_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                img_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<img src=\"" + src + "\" alt=\"" + alt + "\"";
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : img_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>';

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('!');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, href, title;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                href = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, pos - i);
                            }
                            else
                            {
                                pos = bound;
                                href = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<a href=\"" + href + '"';
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = i = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, refid;

                        {
                            size_t pos = line.find(']', start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '[')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        {
                            size_t pos = line.find(']', i);
                            if(pos == std::string::npos)
                                break;
                            
                            refid = line.substr(++i, pos - i - 1);

                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        if(references.count(refid) == 0)
                            continue;

                        std::string tmp = "<a href=\"" + references[refid][0] + '"';
                        if(references[refid][1].compare("") != 0)
                            tmp += " title=" + references[refid][1];
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t op_pos = line.find("***", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 3] == ' ')
                        {
                            op_pos = line.find("***", op_pos + 3);
                            continue;
                        }

                        cl_pos = line.find("***", op_pos + 3);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 3)
                                break;

                            cl_pos = line.find("***", cl_pos + 3);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 3, "<strong><em>");
                        cl_pos += 9;
                        line.replace(cl_pos, 3, "</em></strong>");

                        op_pos = line.find("***", cl_pos + 14);
                    }
                }

                {
                    size_t op_pos = line.find("**", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 2] == ' ')
                        {
                            op_pos = line.find("**", op_pos + 2);
                            continue;
                        }

                        cl_pos = line.find("**", op_pos + 2);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 2)
                                break;

                            cl_pos = line.find("**", cl_pos + 2);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 2, "<strong>");
                        cl_pos += 6;
                        line.replace(cl_pos, 2, "</strong>");

                        op_pos = line.find("**", cl_pos + 9);
                    }
                }

                {
                    size_t op_pos = line.find("*", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("*", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("*", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("*", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 1, "<em>");
                        cl_pos += 3;
                        line.replace(cl_pos, 1, "</em>");

                        op_pos = line.find("*", cl_pos + 5);
                    }
                }

                {
                    size_t op_pos = line.find("`", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("`", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("`", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("`", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        std::string tmp = line.substr(op_pos, op_pos - cl_pos);
                        replace_all(tmp, "&", "&amp;");
                        replace_all(tmp, "<", "&lt;");
                        replace_all(tmp, ">", "&gt;");
                        line.replace(op_pos, op_pos - cl_pos, tmp);

                        op_pos = line.find("`", op_pos + tmp.length());
                    }
                }

                std::string tmp = "<h" + std::to_string(heading_level);

                for(auto attr : attrs)
                    tmp += ' ' + attr.first + '=' + attr.second;

                tmp += '>' + line.substr(substr_start);
                if(tmp.substr(tmp.size() - 2).compare("  ") == 0)
                    tmp.replace(tmp.size() - 2, 2, "<br>");
                tmp += "</h>";

                if(code_block)
                    out += "<code>" + tmp + "</code>\n";
                else
                    out += tmp + '\n';

                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                break;
            }
            else if(ch == '@')
            {
                std::unordered_map<std::string, std::string> attrs{};
                if(line[i] == '<')
                {
                    for(i++; i < line.length(); i++)
                    {
                        if(line[i] == '>')
                            break;

                        if(ch == ' ' || ch == '\t')
                            continue;

                        std::string key, value;
                        int start_index = i;
                        for(; i < line.length(); i++)
                        {
                            if(line[i] == '=')
                                break;
                        }

                        key = line.substr(start_index, i - start_index);

                        start_index = ++i;
                        for(i++; i < line.length(); i++)
                        {
                            if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                break;
                        }

                        value = line.substr(start_index, i - start_index);

                        attrs.insert(std::pair<std::string, std::string>(key, value));

                        if(line[i] == '>')
                            break;
                    }

                    i++;
                }

				size_t substr_start = line[i] == ' ' ? i + 1 : i;

                {
                    size_t start_pos = i = line.find('!', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string alt, src, title;
                        if(line[++i] != '[')
                            continue;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            alt = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('!', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                src = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, bound - i);
                            }
                            else
                            {
                                pos = bound;
                                src = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> img_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                img_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<img src=\"" + src + "\" alt=\"" + alt + "\"";
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : img_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>';

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('!');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, href, title;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                href = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, pos - i);
                            }
                            else
                            {
                                pos = bound;
                                href = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<a href=\"" + href + '"';
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = i = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, refid;

                        {
                            size_t pos = line.find(']', start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '[')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        {
                            size_t pos = line.find(']', i);
                            if(pos == std::string::npos)
                                break;
                            
                            refid = line.substr(++i, pos - i - 1);

                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        if(references.count(refid) == 0)
                            continue;

                        std::string tmp = "<a href=\"" + references[refid][0] + '"';
                        if(references[refid][1].compare("") != 0)
                            tmp += " title=" + references[refid][1];
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t op_pos = line.find("***", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 3] == ' ')
                        {
                            op_pos = line.find("***", op_pos + 3);
                            continue;
                        }

                        cl_pos = line.find("***", op_pos + 3);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 3)
                                break;

                            cl_pos = line.find("***", cl_pos + 3);
                        }

                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 3, "<strong><em>");
                        cl_pos += 9;
                        line.replace(cl_pos, 3, "</em></strong>");

                        op_pos = line.find("***", cl_pos + 14);
                    }
                }

                {
                    size_t op_pos = line.find("**", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 2] == ' ')
                        {
                            op_pos = line.find("**", op_pos + 2);
                            continue;
                        }

                        cl_pos = line.find("**", op_pos + 2);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 2)
                                break;

                            cl_pos = line.find("**", cl_pos + 2);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 2, "<strong>");
                        cl_pos += 6;
                        line.replace(cl_pos, 2, "</strong>");

                        op_pos = line.find("**", cl_pos + 9);
                    }
                }

                {
                    size_t op_pos = line.find("*", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("*", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("*", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("*", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 1, "<em>");
                        cl_pos += 3;
                        line.replace(cl_pos, 1, "</em>");

                        op_pos = line.find("*", cl_pos + 5);
                    }
                }

                {
                    size_t op_pos = line.find("`", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("`", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("`", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("`", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        std::string tmp = line.substr(op_pos, op_pos - cl_pos);
                        replace_all(tmp, "&", "&amp;");
                        replace_all(tmp, "<", "&lt;");
                        replace_all(tmp, ">", "&gt;");
                        line.replace(op_pos, op_pos - cl_pos, tmp);

                        op_pos = line.find("`", op_pos + tmp.length());
                    }
                }

                std::string tmp = "<p";

                for(auto attr : attrs)
                    tmp += ' ' + attr.first + '=' + attr.second;

                tmp += '>' + line.substr(substr_start);
                if(tmp.substr(tmp.size() - 2).compare("  ") == 0)
                    tmp.replace(tmp.size() - 2, 2, "<br>");
                tmp += "</p>";

                if(code_block)
                    out += "<code>" + tmp + "</code>\n";
                else
                    out += tmp + '\n';

                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                break;
            }      
            else
            {
                std::unordered_map<std::string, std::string> attrs{};
                if(line[i] == '<')
                {
                    for(i++; i < line.length(); i++)
                    {
                        if(line[i] == '>')
                            break;

                        if(ch == ' ' || ch == '\t')
                            continue;

                        std::string key, value;
                        int start_index = i;
                        for(; i < line.length(); i++)
                        {
                            if(line[i] == '=')
                                break;
                        }

                        key = line.substr(start_index, i - start_index);

                        start_index = ++i;
                        for(i++; i < line.length(); i++)
                        {
                            if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                break;
                        }

                        value = line.substr(start_index, i - start_index);

                        attrs.insert(std::pair<std::string, std::string>(key, value));

                        if(line[i] == '>')
                            break;
                    }

                    i++;
                }

				size_t substr_start = line[i] == ' ' ? i + 1 : i;

                {
                    size_t start_pos = i = line.find('!', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string alt, src, title;
                        if(line[++i] != '[')
                            continue;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            alt = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('!', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                src = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, bound - i);
                            }
                            else
                            {
                                pos = bound;
                                src = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> img_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                img_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<img src=\"" + src + "\" alt=\"" + alt + "\"";
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : img_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>';

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('!');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, href, title;

                        {
                            size_t pos = line.find("]", start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '(')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        size_t bound = line.find(')', i);
                        if(bound == std::string::npos)
                            break;

                        {
                            size_t pos = line.find(' ', ++i);
                            if(pos != std::string::npos && pos < bound)
                            {
                                href = line.substr(i, pos - i);
                                i = pos + 1;
                                pos = bound;
                                title = line.substr(i, pos - i);
                            }
                            else
                            {
                                pos = bound;
                                href = line.substr(i, pos - i);
                            }
                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        std::string tmp = "<a href=\"" + href + '"';
                        if(title.compare("") != 0)
                            tmp += " title=" + title;
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t start_pos = i = line.find('[', i);
                    while(start_pos != std::string::npos)
                    {
                        std::string text, refid;

                        {
                            size_t pos = line.find(']', start_pos);
                            if(pos == std::string::npos)
                                break;
                            text = line.substr(++i, pos - i - 1);
                            i = pos + 1;
                        }

                        if(line[i] != '[')
                        {
                            start_pos = i = line.find('[', i);
                            continue;
                        }

                        {
                            size_t pos = line.find(']', i);
                            if(pos == std::string::npos)
                                break;
                            
                            refid = line.substr(++i, pos - i - 1);

                            i = pos + 1;
                        }

                        std::unordered_map<std::string, std::string> link_attrs{};
                        if(line[i] == '<')
                        {
                            for(i++; i < line.length(); i++)
                            {
                                if(line[i] == '>')
                                    break;

                                if(ch == ' ' || ch == '\t')
                                    continue;

                                std::string key, value;
                                int start_index = i;
                                for(; i < line.length(); i++)
                                {
                                    if(line[i] == '=')
                                        break;
                                }

                                key = line.substr(start_index, i - start_index);

                                start_index = i + 1;
                                for(i++; i < line.length(); i++)
                                {
                                    if(line[i] == ' ' || line[i] == '\t' || line[i] == '>')
                                        break;
                                }

                                value = line.substr(start_index, i - start_index);

                                link_attrs.insert(std::pair<std::string, std::string>(key, value));

                                if(line[i] == '>')
                                    break;
                            }

                            i++;
                        }

                        if(references.count(refid) == 0)
                            continue;

                        std::string tmp = "<a href=\"" + references[refid][0] + '"';
                        if(references[refid][1].compare("") != 0)
                            tmp += " title=" + references[refid][1];
                        for(auto attr : link_attrs)
                            tmp += ' ' + attr.first + '=' + attr.second;
                        tmp += '>' + text + "</a>";

                        line.replace(start_pos, i - start_pos , tmp);

                        start_pos = line.find('[');
                        i = start_pos + tmp.size();
                    }

                    i = substr_start;
                }

                {
                    size_t op_pos = line.find("***", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 3] == ' ')
                        {
                            op_pos = line.find("***", op_pos + 3);
                            continue;
                        }

                        cl_pos = line.find("***", op_pos + 3);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 3)
                                break;

                            cl_pos = line.find("***", cl_pos + 3);
                        }

                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 3, "<strong><em>");
                        cl_pos += 9;
                        line.replace(cl_pos, 3, "</em></strong>");

                        op_pos = line.find("***", cl_pos + 14);
                    }
                }

                {
                    size_t op_pos = line.find("**", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 2] == ' ')
                        {
                            op_pos = line.find("**", op_pos + 2);
                            continue;
                        }

                        cl_pos = line.find("**", op_pos + 2);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 2)
                                break;

                            cl_pos = line.find("**", cl_pos + 2);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 2, "<strong>");
                        cl_pos += 6;
                        line.replace(cl_pos, 2, "</strong>");

                        op_pos = line.find("**", cl_pos + 9);
                    }
                }

                {
                    size_t op_pos = line.find("*", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("*", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("*", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("*", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        line.replace(op_pos, 1, "<em>");
                        cl_pos += 3;
                        line.replace(cl_pos, 1, "</em>");

                        op_pos = line.find("*", cl_pos + 5);
                    }
                }

                {
                    size_t op_pos = line.find("`", i), cl_pos;
                    while(op_pos != std::string::npos)
                    {
                        if(line[op_pos + 1] == ' ')
                        {
                            op_pos = line.find("`", op_pos + 1);
                            continue;
                        }

                        cl_pos = line.find("`", op_pos + 1);
                        while(cl_pos != std::string::npos)
                        {
                            if(line[cl_pos - 1] != ' ' && cl_pos != op_pos + 1)
                                break;

                            cl_pos = line.find("`", cl_pos + 1);
                        }
                        if(cl_pos == std::string::npos)
                            break;

                        std::string tmp = line.substr(op_pos, op_pos - cl_pos);
                        replace_all(tmp, "&", "&amp;");
                        replace_all(tmp, "<", "&lt;");
                        replace_all(tmp, ">", "&gt;");
                        line.replace(op_pos, op_pos - cl_pos, tmp);

                        op_pos = line.find("`", op_pos + tmp.length());
                    }
                }

                std::string tmp = "<p";

                for(auto attr : attrs)
                    tmp += ' ' + attr.first + '=' + attr.second;

                tmp += '>' + line.substr(substr_start);
                if(tmp.substr(tmp.size() - 2).compare("  ") == 0)
                    tmp.replace(tmp.size() - 2, 2, "<br>");
                tmp += "</p>";

                if(code_block)
                    out += "<code>" + tmp + "</code>\n";
                else
                    out += tmp + '\n';

                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                break;
            }
        }

        if(li == lines.size() - 1)
        {
            if(list_type == UNORDERED_LIST)
            {
                out += "</ul>\n";
                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                list_type = NO_LIST;
            }

            if(list_type == ORDERED_LIST)
            {
                out += "</ol>\n";
                for(size_t j = 0; j < element_stack.size() + 1; j++)
                    out += '\t';

                list_type = NO_LIST;
            }
        }
    }

    out.replace(out.size() - 1, 1, "</html>");

    printf("%s\n", out.c_str());

    return 0;
}
