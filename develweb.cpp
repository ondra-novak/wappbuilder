
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
	void create_dep_file(const std::string &depfile, const std::string &target, bool collapsed);
	void parse_lang_file(const std::string &langfile);
	void walk_includes(std::vector<std::string> &container, const std::string &fname);



protected:
	std::vector<std::string> scripts, styles, templates, header;
	std::set<std::string> includes;
	std::map<std::string, std::string> langfile;
	std::string html_name;
	std::string css_name;
	std::string js_name;
	std::string root_dir;
	std::string charset;
	std::string entry_point;

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
std::string strip_dir(const std::string &name) {
	auto pos = name.rfind(path_separator);
	if (pos == name.npos) return std::string();
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
	if (abs_path.substr(0,dir.length()) == dir) return abs_path.substr(dir.length());
	else return std::string(&path_separator,1)+abs_path;
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
	if (langfile.empty()) {
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
		} else if (checkKw("!css",line)) {
			css_name = line;
		} else if (checkKw("!js",line)) {
			js_name = line;
		} else if (checkKw("!dir",line)) {
			root_dir = line;
		} else if (checkKw("!charset",line)) {
			charset = line;
		} else if (checkKw("!entry_point",line)) {
			entry_point= line;
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
				throw std::runtime_error("Cannot find module: "+ name);
		}
	}

}
void Builder::build(std::ostream &output) {

	output << "<!DOCTYPE html><html><head>" << std::endl;
	for (auto &&x: styles) {
		output << "<link href=\"" << abs_to_rel(root_dir,x) << "\" rel=\"stylesheet\" type=\"text/css\" />" << std::endl;
 	}
	for (auto &&x: header) {
		includeFile(output, x);
		output << std::endl;
 	}
	if (!charset.empty()) {
		output << "<meta charset=\"" << charset << "\" />" << std::endl;
	}
	output << "</head>" << std::endl;

	if (entry_point.empty()) {
		output << "<body>"<< std::endl;
	} else {
		output << "<body onload=\"" << entry_point << "\">"<< std::endl;\
	}

	for (auto &&x: templates) {
		includeFile(output, x);
		output << std::endl;
 	}
	for (auto &&x: scripts) {
		output << "<script src=\"" << abs_to_rel(root_dir,x) << "\" type=\"text/javascript\"/></script>" << std::endl;
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


void Builder::create_dep_file(const std::string& depfile, const std::string& target, bool collapsed) {

	if (target.empty()) return create_dep_file(depfile, rel_to_abs(root_dir, html_name), collapsed);

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
		f << std::endl;
	}

}

inline void Builder::parse_page_file(const std::string& name) {
	root_dir = dirname(name);
	std::string fname = strip_dir(name);
	std::string basename = strip_ext(fname);
	html_name = basename+".html";
	css_name = basename+".css";
	js_name = basename+".js";
	parse_file(name);
}

inline void Builder::build_output() {
	std::string outname = rel_to_abs(root_dir, html_name);
	std::fstream outf(outname, std::ios::out|std::ios::trunc);
	if (!outf) error_writing(outname);
	build(outf);
	if (!outf) error_writing(outname);
}
void Builder::error_writing(const std::string& file) {
	throw std::runtime_error("Error opening (writing) the file: " + file);
}

void Builder::error_reading(const std::string& file) {
	throw std::runtime_error("Error opening (reading) the file: " + file);
}

inline void Builder::parse_lang_file(const std::string& file) {
	if (includes.insert(file).second) {
		std::fstream f(file);
		if (!f) {
			error_reading(file);
		} else {
			std::string ln;
			while (!!f) {
				std::getline(f,ln);
				ln = trim(ln, isspace);
				if (ln.empty() || ln[0] == '#') continue;
				else if (checkKw("$include", ln)) parse_lang_file(rel_to_abs(dirname(file),ln));
				else {
					auto p = ln.find('=');
					std::string key = ln.substr(0,p);
					std::string value = ln.substr(p+1);
					langfile[key] = value;
				}
			}
		}
		includes.erase(file);
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
		std::string infile;
		const char *sw_end="e";

		bool collapse = false;
		bool nooutput = false;

		const char *x = nextParam(false);
		while (x) {
			if (*x == '-') {
				x++;
				do {
					switch(*x) {
					case 'c': collapse = true;break;
					case 'x': nooutput = true;break;
					case 'd': dep_file = nextParam(true);x = sw_end; break;
					case 't': dep_target = nextParam(true);x = sw_end; break;
					case 'l': lang_file = nextParam(true);x = sw_end ;break;
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
				std::cerr << "Usage: " << std::endl
						<<std::endl
						<< "\t" << argv[0] << " -" <<std::endl
						<<std::endl
						<< "-i  <file>      read script from the file. If missing, stdin is read" <<std::endl
						<< "-o  <file>      write result html to the file. If missing, stdout is written unless -n is specified" <<std::endl
						<< "-l  <file>      read stringtable from the file" <<std::endl
						<< "-c  <name>      collapse styles to <name>.css and scripts to <name>.js" <<std::endl
						<< "-d  <file>      write makefile's dependencies to the file" <<std::endl
						<< "-t  <text>      specify dependency target, need when -o is not given" <<std::endl
						<< "-n              do not generate output html" <<std::endl
						<< "-D  <dir>       specify working directory" <<std::endl
						<< std::endl;

				return 1;
		}

		Builder builder;

		if (!lang_file.empty()) {
			builder.parse_lang_file(lang_file);
		}

		builder.parse_page_file(infile);

		if (!dep_file.empty()) {
			builder.create_dep_file(dep_file, dep_target, collapse);
		}


		if (!nooutput) {
			if (collapse) builder.collapse_externals();
			builder.build_output();
		}

	} catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what();
		return 5;
	}

}

