
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

#ifdef _WIN32
	static char path_separator='\\';
#else
	static char path_separator='/';
#endif

	template<typename Fn>
	static std::string trim(const std::string &src, Fn && fn) {
		if (src.empty()) return src;
		auto b = src.begin();
		auto e = src.end();
		while (b != e && fn(*b)) ++b;
		while (b != e) {
			--e;
			if (!fn(*e)) {
				++e;
				break;
			}
		}
		return std::string(b,e);
	}


class Builder {
public:


	void parse_page_file(const std::string &name);
	void build_output();
	void collapse_externals();
	void create_dep_file(const std::string &depfile, const std::string &target, bool collapsed, bool phony);
	void parse_lang_file(const std::string &langfile);
	void gen_lang_file(const std::string &langfile);
	void walk_includes(std::vector<std::string> &container, const std::string &fname);
	void set_root_dir(const std::string &root_dir);
	void set_base_name(const std::string &out_file);



protected:
	std::vector<std::string> scripts, styles, templates, header;
	std::set<std::string> includes;
	std::map<std::string, std::string> langfile;
	std::set<std::string> missing_lang;
	std::string html_name;
	std::string css_name;
	std::string js_name;
	std::string root_dir;
	std::string charset;
	std::string entry_point;
	std::string prefix;
	std::string lang_file_name
	;
	bool async_css = false;
	bool async_script = false;
	bool hasLang = false;
	bool override_html_name = false;
	bool override_css_name = false;
	bool override_js_name = false;

	static bool try_ext(const std::string &line, const char *ext, std::string &fullname);
	void includeFile(std::ostream &out, const std::string &fname);

	void collapse(std::vector<std::string> &block, const std::string &outfile);
	void parse(const std::string &dir, std::istream &input);
	void build(std::ostream &output);
	void parse_file(const std::string &fname);

private:
	static void error_reading(const std::string& file);
	static void error_writing(const std::string& file);
	template<typename Out>
	void translate_file(std::ifstream &in, Out &&out);
	template<typename Out>
	void scan_variable(std::istream& in, Out &&out);
};

class OStreamOut {
public:

	OStreamOut(std::ostream &out):out(out) {}
	void operator()(char c) const {out.put(c);}
protected:
	std::ostream &out;
};

std::string dirname(const std::string &name) {
	auto pos = name.rfind(path_separator);
	if (pos == name.npos) return std::string();
	else return name.substr(0,pos+1);
}
std::string pardirname(const std::string &name) {
	if (name.empty()) return name;
	if (name[name.length()-1] == '/') return pardirname(name.substr(0,name.length()-1));
	return dirname(name);
}

std::string strip_dir(const std::string &name) {
	auto pos = name.rfind(path_separator);
	if (pos == name.npos) return name;
	else return name.substr(pos+1);
}
std::string strip_ext(const std::string &name) {
	auto pos = name.rfind('.');
	if (pos == name.npos) return name;
	else return name.substr(0,pos);
}

std::string rel_to_abs(const std::string &dir, const std::string relpath) {
	if (relpath.empty()) return dir;
	if (dir.empty() || relpath[0] == path_separator) return relpath;
	std::string res;
	res.reserve(dir.length()+relpath.length()+1);
	res.append(dir);
	if (res[res.length()-1] != path_separator) res.push_back(path_separator);
	res.append(relpath);
	return res;
}
std::string abs_to_rel(const std::string &dir, const std::string abs_path) {
	if (abs_path.empty()) return dir;
	if (abs_path[0] == path_separator) return abs_path;
	if (dir.empty()) return abs_path;
	if (dir[dir.length()-1] != '/') return abs_to_rel(dir+"/", abs_path);
	if (abs_path.substr(0,dir.length()) == dir) return abs_path.substr(dir.length());
	else {
		std::string up = "..";
		up.push_back(path_separator);
		up.append(abs_path);
		return abs_to_rel(pardirname(dir),up);
	}
}

void changeDir(const std::string &dir) {
	if (!dir.empty()) {
		if (chdir(dir.c_str())) {
			throw std::runtime_error("Can't change to directory: " + dir);
		}
	}
}

bool checkKw(const char *name, std::string &dir) {
	std::string::size_type idx = 0;
	std::string::size_type len = dir.length();
	while (name[idx] && idx<len) {
		if (name[idx] != dir[idx]) return false;
		++idx;
	}
	if (idx == len || !isspace(dir[idx])) return false;
	dir = trim(dir.substr(idx),isspace);
	return true;
}


void Builder::parse_file(const std::string &fname) {
	std::ifstream f(fname);
	if (!f) {
		error_reading(fname);
	}
	if (!hasLang) {
		parse(dirname(fname),f);
	} else {
		std::ostringstream tmpfile;
		translate_file(f,OStreamOut(tmpfile));
		std::istringstream rdtmpfile(tmpfile.str());
		parse(dirname(fname), rdtmpfile);
	}
}


void Builder::parse(const std::string &dir, std::istream &input) {

	std::string line,name;
	while (!input.eof()) {

		std::getline(input,line);
		line = trim(line,isspace);
		if (line.empty() || line[0] == '#') {
			continue;
		} else if (checkKw("!include",line)) {
			std::string fname = dir+line.substr(9);
			parse_file(fname);
		} else if (checkKw("!html",line)) {
			html_name = line;
			override_html_name = true;
		} else if (checkKw("!css",line)) {
			css_name = line;
			override_css_name = true;
		} else if (checkKw("!js",line)) {
			js_name = line;
			override_js_name = true;
		} else if (checkKw("!dir",line)) {
			root_dir = line;
		} else if (checkKw("!charset",line)) {
			charset = line;
		} else if (checkKw("!entry_point",line)) {
			entry_point= line;
		} else if (checkKw("!defer_script",line)) {
			async_script = line == "true" || line == "yes";
		} else if (checkKw("!defer_css",line)) {
			async_css = line == "true" || line == "yes";
		} else {
			std::string fpath = rel_to_abs(dir,line);
			bool ok = false;
			if (try_ext(fpath, ".html", name)) {
				walk_includes(templates,name);
				ok = true;
			}
			if (try_ext(fpath, ".htm", name)) {
				walk_includes(templates,name);
				ok = true;
			}
			if (try_ext(fpath, ".css", name)) {
				ok = true;
				walk_includes(styles, name);
			}
			if (try_ext(fpath, ".js", name)) {
				ok = true;
				walk_includes(scripts,name);
			}
			if (try_ext(fpath, ".hdr", name)) {
				ok = true;
				walk_includes(header, name);
			}
			if (!ok)
				throw std::runtime_error("Cannot find module: "+ line + ".*");
		}
	}

}
void Builder::build(std::ostream &output) {

	output << "<!DOCTYPE html><html><head>" << std::endl;
	if (!async_css) {
		for (auto &&x: styles) {
			output << "<link href=\"" << abs_to_rel(root_dir,x) << "\" rel=\"stylesheet\" type=\"text/css\" />" << std::endl;
		}
	}
	for (auto &&x: header) {
		includeFile(output, x);
		output << std::endl;
 	}
	if (!charset.empty()) {
		output << "<meta charset=\"" << charset << "\" />" << std::endl;
	}
	output << "</head>" << std::endl;

	output << "<body>"<< std::endl;

	for (auto &&x: templates) {
		includeFile(output, x);
		output << std::endl;
 	}
	for (auto &&x: scripts) {
		output << "<script "<< (async_script?"defer":"") << " src=\"" << abs_to_rel(root_dir,x) << "\" type=\"text/javascript\"/></script>" << std::endl;
 	}
	if (async_css || !entry_point.empty()) {
		output << "<script type=\"text/javascript\">"<< std::endl;
		output << "document.addEventListener(\"DOMContentLoaded\",function(){\"use strict\";";

		if (async_css) {

			output << "var counts = 0;[";


			const char *sep = "";
			for (auto &&x: styles) {
				output << sep << '"' << abs_to_rel(root_dir, x) << '"';
				sep = ",";
			}
			output << "].forEach(function(x,p,arr) {"
					"var f = document.createElement( \"link\" );"
					"f.rel = \"stylesheet\";"
					"f.href = x;"
					"f.addEventListener(\"load\",function(){counts++;if (counts == arr.length) document.dispatchEvent( new Event(\"styles_loaded\"));});"
					"document.head.appendChild(f);});";

		}
		if (!entry_point.empty()) {
			output << entry_point << ";";
		}
		output << "});"<<std::endl << "</script>" << std::endl;

	}
	output << "</body></html>";
	output << std::endl;

}

bool Builder::try_ext(const std::string& line, const char* ext,	std::string& fullname) {
	fullname = line + ext;
	std::fstream probe(fullname);
	if (!probe) return false;
	return true;
}

void Builder::collapse_externals() {
	collapse(styles, rel_to_abs(root_dir, css_name));
	collapse(scripts, rel_to_abs(root_dir, js_name));
}


void Builder::create_dep_file(const std::string& depfile, const std::string& target, bool collapsed, bool phony) {

	if (target.empty()) return create_dep_file(depfile, rel_to_abs(root_dir, html_name), collapsed, phony);

	std::ofstream f(depfile, std::ios::trunc|std::ios::out);
	if (!f) {
		std::cerr << "Error writing to file: " << depfile << std::endl;
	} else {
		f << target << " " <<  depfile << " :";
		std::initializer_list<const std::vector<std::string> *> list_collapsed{
				&templates, &styles, &scripts, &header,
		};
		std::initializer_list<const std::vector<std::string> *> list_debug{
				&templates, &header,
		};
		auto &list=collapsed?list_collapsed:list_debug;
		for (auto &&y: list ) {
			for (auto &&x : *y) f << "\\" << std::endl  << x;
		}
		if (!lang_file_name.empty())
				f << "\\" << std::endl  << lang_file_name;
		f << std::endl;
		if (phony) {
			for (auto &&y: list ) {
				for (auto &&x : *y) {
					f << std::endl << x << ":" << std::endl;
				}
			}
			if (!lang_file_name.empty())
				f << std::endl << lang_file_name << ":" << std::endl;
		}
	}

}

void Builder::set_base_name(const std::string &basename) {
	if (!override_html_name) html_name = basename+".html";
	if (!override_css_name) css_name = basename+".css";
	if (!override_js_name) js_name = basename+".js";
}

inline void Builder::parse_page_file(const std::string& name) {
	root_dir = dirname(name);
	std::string fname = strip_dir(name);
	std::string basename = strip_ext(fname);
	set_base_name(basename);
	parse_file(name);
/*	std::cout << root_dir << std::endl
			<< html_name << std::endl
			<< css_name << std::endl
			<< js_name << std::endl
			<< fname << std::endl
			<< basename << std::endl
			<< name << std::endl;*/
}

inline void Builder::build_output() {
	std::string outname = rel_to_abs(root_dir, html_name);
	std::fstream outf(outname, std::ios::out|std::ios::trunc);
	if (!outf) error_writing(outname);
	build(outf);
	if (!outf) error_writing(outname);
}

inline void Builder::set_root_dir(const std::string& root_dir) {
	this->root_dir = root_dir;
}

void Builder::error_writing(const std::string& file) {
	throw std::runtime_error("Error opening (writing) the file: " + file);
}

void Builder::error_reading(const std::string& file) {
	throw std::runtime_error("Error opening (reading) the file: " + file);
}

namespace CSV {

void read_except(std::istream &f , const char *seq) {
	int c = f.get();
	while (c != EOF && isspace(c) && c != *seq) {
		c = f.get();
	}
	while (*seq) {
		if (c != *seq) throw std::runtime_error("Lang file parse error ");
		seq++;
		c = f.get();
	}
	f.putback(c);

}

void write_string(std::ostream &f, const std::string &s) {
	f.put('"');
	for (auto &&x: s) {
		if (x == '"') f << "\"\"";
		else f.put(x);
	}
	f.put('"');
}

void read_string (std::istream &f, std::string &str) {
	read_except(f, "\"");
	str.clear();
	int c = f.get();
	do {
		while (c != EOF && c != '"') {
			str.push_back(c);
			c = f.get();
		}
		if (c == '"') {
			char d = f.get();
			if (d != c) {
				f.putback(d);
				return;
			} else {
				str.push_back(c);
			}
		} else {
			return;
		}
	} while (true);
}


}

inline void Builder::parse_lang_file(const std::string& file) {

	hasLang = true;

	using namespace CSV;

	std::ifstream f(file);

	if (!f) {
		error_reading(file);
	}

	this->lang_file_name = file;



	std::string ns;
	std::string orgtext;
	std::string translated_text;
	std::string key;
	while (!!f) {

		read_string(f, ns);
		read_except(f,",");
		read_string(f, orgtext);
		read_except(f,",");
		read_string(f, translated_text);
		read_except(f,"\r\n");

		key.clear();
		if (!ns.empty()) {
			key.append(ns);
			key.append("::");
		}
		key.append(orgtext);
		langfile[key] = translated_text;

		int c = f.get();
		while (c != EOF && isspace(c)) c = f.get();;
		if (c != EOF) f.putback(c);
	}
}

void Builder::collapse(std::vector<std::string>& block, const std::string& outfile) {
	std::ofstream f(outfile, std::ios::trunc|std::ios::out);
	if (!f) {
		std::cerr << "Error writing to file: " << outfile << std::endl;
	} else {
		for (auto &&x : block) {
			includeFile(f, x);
		}
		f << std::endl;
	}
	block.clear();
	block.push_back(outfile);
}


template<typename Out>
void Builder::scan_variable(std::istream& in, Out && out) {
	std::string buffer;
	int i = in.get();
	if (i == '{') {
		buffer.push_back(i);
		i = in.get();
		if (i == '{') {
			buffer.push_back(i);
			i = in.get();
			while (i != '}' && i != EOF) {
				buffer.push_back(i);
				i = in.get();
			}
			if (i == '}') {
				buffer.push_back(i);
				i = in.get();
				if (i == '}') {
					buffer.push_back(i);
					std::string name = buffer.substr(2,buffer.length()-4);
					auto iter = langfile.find(name);
					const std::string *writeout;
					if (iter != langfile.end()) {
						writeout = &iter->second;
					} else {
						missing_lang.insert(name);
						auto pos = name.rfind("::");
						if (pos != name.npos)
							name = name.substr(pos+2);
						writeout = &name;
					}
					for (auto && c: *writeout) out(c);
					return;
				}
			}
		}
	}
	for (auto &&x : buffer) out(x);
	if (i != EOF) {
		in.putback(i);
	}
}

template<typename Out>
void Builder::translate_file(std::ifstream &in, Out&& out) {
	int i = in.get();
	while (i != EOF) {
		if (i == '{') {
			in.putback(i);
			scan_variable(in, out);
		} else {
			out(i);
		}
		i = in.get();
	}
}

void Builder::includeFile(std::ostream& out, const std::string& fname) {
	std::ifstream in(fname);
	if (!in) {
		error_reading(fname);
	} else {
		translate_file(in, OStreamOut(out));
	}
}

void Builder::walk_includes(std::vector<std::string> &container, const std::string &fname) {
	if (includes.insert(fname).second) {
		std::ifstream f(fname);
		if (!f)  {
			error_reading(fname);
		}

		std::string line;

		while (!!f) {
			std::getline(f,line);
			std::string tline(trim(line, isspace));
			if (checkKw("//!require", tline)) {
				walk_includes(container, rel_to_abs(dirname(fname), tline));
			}
			if (checkKw("/*!require", tline)) {
				tline = trim(tline.substr(0,tline.length()-2),isspace);
				walk_includes(container, rel_to_abs(dirname(fname), tline));

			}
			if (checkKw("<!--!require", tline)) {
				tline = trim(tline.substr(0,tline.length()-3),isspace);
				walk_includes(container, rel_to_abs(dirname(fname), tline));

			}
		}

		container.push_back(fname);
	}

}

void Builder::gen_lang_file(const std::string &langfile) {
	using namespace CSV;
	std::ofstream out(langfile, std::ios::out| std:: ios::trunc);
	if (!out) error_writing(langfile);
	for (auto &&x: missing_lang) {
		auto pos = x.rfind("::");
		if (pos == x.npos) {
			out << "\"\",";
			write_string(out, x);
		} else {
			write_string(out, x.substr(0,pos));
			out.put(',');
			write_string(out, x.substr(pos+2));
		}
		out << ",\"\"\r\n";
	}
}



int main(int argc, char **argv) {

	try {
		int idx = 1;
		auto nextParam = [&](bool required) -> const char * {
			if (idx >= argc) {
				if (required) {
					std::cerr << "Missing argument" << std::endl;
					exit(1);
				} else {
					return nullptr;
				}
			} else {
				return argv[idx++];
			}
		};

		std::string dep_file;
		std::string dep_target;
		std::string lang_file;
		std::string gen_lang_file;
		std::string infile;
		std::string root_dir;
		std::string base_name;
		const char *sw_end="e";

		bool collapse = false;
		bool nooutput = false;
		bool phony = false;

		const char *x = nextParam(false);
		while (x) {
			if (*x == '-') {
				x++;
				do {
					switch(*x) {
					case 'c': collapse = true;break;
					case 'x': nooutput = true;break;
					case 'p': phony = true;break;
					case 'D': root_dir = nextParam(true);x = sw_end; break;
					case 'd': dep_file = nextParam(true);x = sw_end; break;
					case 't': dep_target = nextParam(true);x = sw_end; break;
					case 'L': lang_file = nextParam(true);x = sw_end; break;
					case 'G': gen_lang_file = nextParam(true);x = sw_end; break;
					case 'B': base_name = nextParam(true);x = sw_end; break;
					case 'l':
						std::cerr << "Switch -l is no longer supported " << x << std::endl;
						return 1;
					default:
						std::cerr << "Unknown switch " << x << std::endl;
						return 1;
					}
				} while (*(++x));
			} else {
				if (infile.empty())
					infile = x;
				else
					std::cerr << "Only one file can be input file" << std::endl;

			}
			x = nextParam(false);
		}

		if (infile.empty()) {
				std::cerr << "Copyright (c) 2018 Ondrej Novak" << std::endl
						<< std::endl
						<< "Permission is hereby granted, free of charge, to any person"<< std::endl
						<< "obtaining a copy of this software and associated documentation"<< std::endl
						<< "files (the \"Software\"), to deal in the Software without"<< std::endl
						<< "restriction, including without limitation the rights to use,"<< std::endl
						<< "copy, modify, merge, publish, distribute, sublicense, and/or sell"<< std::endl
						<< "copies of the Software, and to permit persons to whom the"<< std::endl
						<< "Software is furnished to do so, subject to the following"<< std::endl
						<< "conditions:"<< std::endl
						<< std::endl
						<< "The above copyright notice and this permission notice shall be"<< std::endl
						<< "included in all copies or substantial portions of the Software."<< std::endl
						<< std::endl
						<< "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,"<< std::endl
						<< "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES"<< std::endl
						<< "OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND"<< std::endl
						<< "NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT"<< std::endl
						<< "HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,"<< std::endl
						<< "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING"<< std::endl
						<< "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR"<< std::endl
						<< "OTHER DEALINGS IN THE SOFTWARE."<< std::endl<< std::endl
						<< "Usage: " << std::endl
						<<std::endl
						<< argv[0] << " [-c][-x][-p][-d <depfile>][-l <langfile>][-t <target>] <input.page>" <<std::endl
						<<std::endl
						<< "<input.page>    file contains commands and references to various modules (described below)"<<std::endl
						<< "-d  <depfile>   generated dependency file (for make)" <<std::endl
						<< "-t  <target>    target in dependency file. If not specified, it is determined from the script" <<std::endl
						<< "-p              add phony targets to dep file" << std::endl
						<< "-L  <langfile>  language file (csv)" <<std::endl
						<< "-G  <langfile>  output generated lang file (csv)" <<std::endl
						<< "-B  <name>      override basename"<<std::endl
						<< "-x              do not generate output. Useful with -d (-xd depfile)" <<std::endl
						<< "-c              collapse scripts and styles into single file(s)" <<std::endl
						<< std::endl
						<< "Page file format" <<std::endl
						<< std::endl
						<< "A common text file where each command is written to a separate line." << std::endl
						<< "Each the line can be either empty, or contain a comment starting by #," << std::endl
						<< "or can be a directive which starts with !, or a relative path to a module." << std::endl
						<< std::endl
						<< "Module: One or more files where the name (without extension) is the same." << std::endl
						<< "The extension specifies type of resource tied to the module" << std::endl
						<< "   - <name>.html : part of HTML code" << std::endl
						<< "   - <name>.htm : part of HTML code" << std::endl
						<< "   - <name>.css : part of CSS code" << std::endl
						<< "   - <name>.js : part of JS code" << std::endl
						<< "   - <name>.hdr : part of HTML code which is put to the header section" << std::endl
						<< "At least one file of the module must exist." << std::endl
						<< std::endl
						<< "Directives" <<std::endl
						<< std::endl
						<< "!include <file>    - include file to the script. "<< std::endl
						<< "                       The path is relative to the current file" <<std::endl
						<< "!html <file>       - specify output html file (override default settins)." <<std::endl
						<< "                       The path is relative to the root script" <<std::endl
						<< "!css <file>        - specify output css file (override default settins)." <<std::endl
						<< "                       The path is relative to the root script" <<std::endl
						<< "!js <file>         - specify output js file (override default settins)" <<std::endl
						<< "                       The path is relative to the root script" <<std::endl
						<< "!dir <file>        - change directory of root " <<std::endl
						<< "                       The path is relative to the root script" <<std::endl
						<< "!charset <cp>    of )  - set charset of the output html page" <<std::endl
						<< "!entry_point <fn>  - specify function used as entry point. " <<std::endl
						<< "                       You need to specify complete call, including ()" <<std::endl
						<< "                       example: '!entry_point main()'" <<std::endl
						<< "!defer_script yes  - script is loaded with defer flag. " <<std::endl
						<< "!defer_css yes     - styles are loaded after scripts. " <<std::endl
						<< std::endl
						<< "In source references" <<std::endl
						<< std::endl
						<< "Inside of source files, there can be reference to other source files" << std::endl
						<< "You can put reference as comment using directive !require" << std::endl
						<< std::endl
						<< "to javascript: //!require <file.js>" << std::endl
						<< "to css: /*!require <file.js>*/" << std::endl
						<< "to html: <!--!require <file.js>-->" << std::endl
						<< std::endl
						<< "referenced files are also included to the project. However circular" << std::endl
						<< "references are not allowed resulting to break the cycle once it is detected" << std::endl
						<< std::endl
						<< "Language" <<std::endl
						<< std::endl
						<< "You can use placeholder " << std::endl
						<< "   {{text}} " << std::endl
						<< "   {{namespace::text}} " << std::endl
						<< "   {{ns1::ns2::...::text}} "<< std::endl
						<< "instead of text to refer text in the language file " <<std::endl
						<< std::endl
						<< "Use -G to generate the language file" <<std::endl;


				return 1;
		}

		Builder builder;

		if (!lang_file.empty()) {
			try {
				builder.parse_lang_file(lang_file);
			} catch (std::exception &e) {
				std::cerr << "Warning: " << e.what() << std::endl;
			}
		}

		builder.parse_page_file(infile);


		if (!base_name.empty()) {
			builder.set_base_name(base_name);
		}

		if (!root_dir.empty()) {
			builder.set_root_dir(root_dir);
		}

		if (!dep_file.empty()) {
			builder.create_dep_file(dep_file, dep_target, true, phony);
		}

		if (!nooutput) {
			if (collapse) builder.collapse_externals();
			builder.build_output();
		}

		if (!gen_lang_file.empty()) {
			builder.gen_lang_file(gen_lang_file);
		}

	} catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return 5;
	}

}

