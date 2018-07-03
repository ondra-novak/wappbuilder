
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

class Builder {
public:


	void parse(const std::string &dir, std::istream &input);
	void parse_file(const std::string &fname);
	void build(std::ostream &output);
	void collapse_styles(const std::string &outfile);
	void collapse_scripts(const std::string &outfile);
	void create_dep_file(const std::string &depfile, const std::string &target);


protected:
	std::vector<std::string> scripts, styles, templates, header;
	std::set<std::string> includes;

	static bool try_ext(const std::string &line, const char *ext, std::string &fullname);
	static void includeFile(std::ostream &out, const std::string &fname);

	void collapse(std::vector<std::string> &block, const std::string &outfile);

};

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

void Builder::parse_file(const std::string &fname) {
	auto sep = fname.rfind('/');
	std::string dir;
	if (sep != fname.npos) {
		dir = fname.substr(0,sep+1);
	}
	std::fstream f(fname);
	if (!f) {
		std::cerr << "Error opening file: " << fname << std::endl;
	}
	parse(dir,f);
}


void Builder::parse(const std::string &dir, std::istream &input) {

	std::string line,name;
	while (!input.eof()) {

		std::getline(input,line);
		line = trim(line,isspace);
		if (line.empty() || line[0] == '#') {
			continue;
		} else if (line.substr(0,9) == "$include ") {
			std::string fname = dir+trim(line.substr(9),isspace);
			parse_file(fname);
		} else {
			std::string fpath = dir + line;
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

void Builder::includeFile(std::ostream& out, const std::string& fname) {
	std::ifstream in(fname);
	if (!in) {
		std::cerr << "Error opening file: " << fname << std::endl;
	} else {
		int i = in.get();
		while (i != EOF) {
			out.put(i);
			i = in.get();
		}
	}
}


int main(int argc, char **argv) {

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
	bool nooutput = false;

	const char *x = nextParam(false);
	while (x) {
		if (x[0] == '-' && x[1] != 0 && x[2] == 0) {
			switch(x[1]) {
			case 'i':infile = nextParam(true);break;
			case 'o': outfile = nextParam(true);break;
			case 'c':collapse_base = nextParam(true);break;
			case 'd': dep_file = nextParam(true);break;
			case 't': dep_target = nextParam(true);break;
			case 'n': nooutput = true;break;
			case 'D': chdir(nextParam(true));break;
			default:
				std::cerr << "Unknown switch " << x << std::endl;
				return 1;
			}
		} else {
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
	if (infile.empty()) {
		builder.parse("./", std::cin);
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


}

