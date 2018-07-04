
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <fstream>
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


	void parse(const std::string &dir, std::istream &input);
	void parse_file(const std::string &fname);
	void build(std::ostream &output);
	void collapse_styles(const std::string &outfile);
	void collapse_scripts(const std::string &outfile);
	void create_dep_file(const std::string &depfile, const std::string &target);
	void parse_lang_file(const std::string &langfile);


protected:
	std::vector<std::string> scripts, styles, templates, header;
	std::set<std::string> includes;
	std::map<std::string, std::string> langfile;

	static bool try_ext(const std::string &line, const char *ext, std::string &fullname);
	void includeFile(std::ostream &out, const std::string &fname);
	void scan_variable(std::istream& in, std::ostream &out);

	void collapse(std::vector<std::string> &block, const std::string &outfile);

private:
	static void error_reading(const std::string& file);
};

std::string dirname(const std::string &name) {
	auto pos = name.rfind(path_separator);
	if (pos == name.npos) return std::string();
	else return name.substr(0,pos+1);
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
	std::fstream f(fname);
	if (!f) {
		error_reading(fname);
	}
	parse(dirname(fname),f);
}


void Builder::parse(const std::string &dir, std::istream &input) {

	std::string line,name;
	while (!input.eof()) {

		std::getline(input,line);
		line = trim(line,isspace);
		if (line.empty() || line[0] == '#') {
			continue;
		} else if (checkKw("$include",line)) {
			std::string fname = dir+trim(line.substr(9),isspace);
			parse_file(fname);
		} else {
			std::string fpath = rel_to_abs(dir,line);
			if (try_ext(fpath, ".html", name)) templates.push_back(name);
			if (try_ext(fpath, ".htm", name)) templates.push_back(name);
			if (try_ext(fpath, ".css", name)) styles.push_back(name);
			if (try_ext(fpath, ".js", name)) scripts.push_back(name);
			if (try_ext(fpath, ".hdr", name)) header.push_back(name);
		}
	}

}
void Builder::build(std::ostream &output) {

	output << "<!DOCTYPE html><html><head>" << std::endl;
	for (auto &&x: styles) {
		output << "<link href=\"" << x << "\" rel=\"stylesheet\" type=\"text/css\" />" << std::endl;
 	}
	for (auto &&x: header) {
		includeFile(output, x);
		output << std::endl;
 	}
	output << "</head><body>";
	for (auto &&x: templates) {
		includeFile(output, x);
		output << std::endl;
 	}
	for (auto &&x: scripts) {
		output << "<script src=\"" << x << "\" type=\"text/javascript\"/></script>" << std::endl;
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

void Builder::collapse_styles(const std::string& outfile) {
	collapse(styles, outfile);
}

void Builder::collapse_scripts(const std::string& outfile) {
	collapse(scripts, outfile);
}

void Builder::create_dep_file(const std::string& depfile, const std::string& target) {
	std::ofstream f(depfile, std::ios::trunc|std::ios::out);
	if (!f) {
		std::cerr << "Error writing to file: " << depfile << std::endl;
	} else {
		f << target << " " <<  depfile << " :";
		std::initializer_list<const std::vector<std::string> *> list{
				&templates, &styles, &scripts, &header,
		};
		for (auto &&y: list ) {
			for (auto &&x : *y) f << "\\" << std::endl  << x;
		}
		f << std::endl;
	}

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


void Builder::scan_variable(std::istream& in, std::ostream &out) {
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
					if (iter != langfile.end()) {
						out << iter->second;
						return;
					}
					i = EOF;
				}
			}
		}
	}
	for (auto &&x : buffer) out.put(x);
	if (i != EOF) {
		in.putback(i);
	}
}
void Builder::includeFile(std::ostream& out, const std::string& fname) {
	std::ifstream in(fname);
	if (!in) {
		error_reading(fname);
	} else {
		int i = in.get();
		while (i != EOF) {
			if (i == '{') {
				in.putback(i);
				scan_variable(in, out);
			} else {
				out.put(i);

			}
			i = in.get();
		}
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

		std::string infile;
		std::string outfile;
		std::string collapse_base;
		std::string dep_file;
		std::string dep_target;
		std::string lang_file;
		bool nooutput = false;

		const char *x = nextParam(false);
		while (x) {
			if (x[0] == '-' && x[1] != 0 && x[2] == 0) {
				switch(x[1]) {
				case 'i':infile = nextParam(true);break;
				case 'o': outfile = nextParam(true);break;
				case 'l': lang_file = nextParam(true);break;
				case 'c':collapse_base = nextParam(true);break;
				case 'd': dep_file = nextParam(true);break;
				case 't': dep_target = nextParam(true);break;
				case 'n': nooutput = true;break;
				case 'D': changeDir(nextParam(true));break;
				default:
					std::cerr << "Unknown switch " << x << std::endl;
					return 1;
				}
			} else {
				std::cerr << "Usage: " << std::endl
						<<std::endl
						<< "\t" << argc[0] << " <...options...>" <<std::endl
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

				std::cerr << "Help TODO" << std::endl;
				return 1;
			}
			x = nextParam(false);
		}

		if (!dep_file.empty() && dep_target.empty()) {
			if (outfile.empty()) {
				std::cerr << "Need specify -o or -t" << std::endl;
				return 1;
			}
			dep_target = outfile;
		}



		Builder builder;

		if (!lang_file.empty()) {
			builder.parse_lang_file(lang_file);
		}

		if (infile.empty()) {
			builder.parse("", std::cin);
		}else{
			builder.parse_file(infile);
		}

		if (!dep_file.empty()) {
			builder.create_dep_file(dep_file, dep_target);
		}

		if (!collapse_base.empty()) {
			builder.collapse_scripts(collapse_base+".js");
			builder.collapse_styles(collapse_base+".css");
		}

		if (!nooutput) {

			if (outfile.empty()) {
				builder.build(std::cout);
			} else {
				std::ofstream f(outfile, std::ios::out|std::ios::trunc);
				if (!f) {
					std::cerr << "Can't write to " << outfile << std::endl;
					return 2;
				}
				builder.build(f);
			}
		}
	} catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what();
		return 5;
	}

}

