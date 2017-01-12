/*
 * Table.h
 *
 *  Created on: Dec 5, 2016
 *      Author: teng
 */

#ifndef INCLUDE_TABLE_H_
#define INCLUDE_TABLE_H_
#include "util.h"
#include "vectorization-ir.h"
#include "llvm-codegen.h"
#include "ORCColumnInfo.h"

using namespace orc;
namespace tengdb{

const static std::string separator = "__";
inline std::string replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return str;
    str.replace(start_pos, from.length(), to);
    return str;
}

class Rep{
private:
	bool isfile = false;
	std::string path;
	std::string filename;
	DIR *dir;
public:
	Rep(std::string path){

		this->path = path;
		int pos = path.find(separator);
		if(pos!=std::string::npos){
			isfile = true;
			filename = path.substr(pos+separator.length(),path.length());
			this->path = path.substr(0,pos);
		}
		dir = NULL;
		reset();
	}

	~Rep(){
		closedir(dir);
	}

	dirent *nextFile(){
		dirent *file = readdir (dir);
		while(true){

			if(file==NULL){
				return NULL;
			}else if(file->d_type == DT_REG&&file->d_name[0]!='.'){
				if(!isfile||std::string(file->d_name)==filename){
					break;
				}
			}
			file = readdir(dir);
		}
	    return file;
	}

	std::string getPath(){
		return path;
	}

	void reset(){
		if(!dir){
			closedir(dir);
			dir = NULL;
		}
		dir = opendir (path.c_str());
		if(dir==NULL){
			printf("%s cannot be opened\n",path.c_str());
			exit(0);
		}
	}

};

class Table{

private:
	int registered;
	std::map<std::string,std::pair<int,TypeKind>> colmap;
	Rep *rep;
	LlvmCodeGen *gen;
	ObjectPool *pool;
	std::string tablename;
	std::mutex mtx;

public:

	Table(std::string tablename, std::string path, std::vector<Column> columns){
		this->tablename = tablename;
		for(int i=0;i<columns.size();i++){
			colmap[columns[i].name] = std::make_pair(i+1,columns[i].type);
		}
		gen = NULL;
		pool = NULL;
		rep = new Rep(path);
		registered = 0;
	}
	int reg(){
		int id = 0;
		mtx.lock();
		id = ++registered;
		mtx.unlock();
		return id;
	}

	int unreg(){
		int id = 0;
		mtx.lock();
		id = --registered;
		mtx.unlock();
		return id;
	}

	int getRegisteredNumber(){
		return registered;
	}

	int getColumnNumber(std::string columnname){
		if(colmap.find(columnname)==colmap.end()){
			printf("%s is not a column!\n",columnname.c_str());
			exit(0);
		}
		return colmap[columnname].first;
	}

	TypeKind getTypeKind(std::string columnname){
		if(colmap.find(columnname)==colmap.end()){
			printf("%s is not a column!\n",columnname.c_str());
			exit(0);
		}
		return colmap[columnname].second;
	}

	LlvmCodeGen *getCodeGen(){

		mtx.lock();
		if(!gen){
			pool = new ObjectPool();
			if(exists_module_file(tablename)&&!FLAGS_codegen){
				if(FLAGS_use_optimized_module){
	            	LlvmCodeGen::LoadFromFile(pool,tablename+"_opt.ll",tablename,&gen);
				}else{
	            	LlvmCodeGen::LoadFromFile(pool,tablename+"_unopt.ll",tablename,&gen);
				}
			}else{
				gen = new LlvmCodeGen(pool,tablename);
				initializeVectorization(gen);
			}
		}
		mtx.unlock();

		return gen;

	}

	~Table(){
		if(rep){
			delete rep;
		}
		if(gen){
			delete gen;
			delete pool;
		}
	}

	void reset(){
		rep->reset();
	}
	Reader* nextFile(){

		mtx.lock();
		dirent *dir = rep->nextFile();
		mtx.unlock();
		if(dir==NULL){
			return NULL;
		}
		ReaderOptions opts;
		return orc::createReader(orc::readLocalFile(rep->getPath()+"/"+dir->d_name),opts);
	}

};


static Table *getTable(std::string table){

	std::string tablename = table;
	if(table.find(separator)!=std::string::npos){
		int pos = table.find(separator);
		tablename = table.substr(0,pos);
		std::string filename = table.substr(pos+separator.length(),table.length());
	}
	std::string tablepath = dbpath+"/"+table;
	std::vector<Column> columns;

	if(tablename=="allsmall" || tablename=="lineitem"){
		columns.push_back(Column("l_orderkey",TypeKind::INT));
		columns.push_back(Column("l_partkey",TypeKind::INT));
		columns.push_back(Column("l_suppkey",TypeKind::INT));
		columns.push_back(Column("l_linenumber",TypeKind::INT));
		columns.push_back(Column("l_quantity",TypeKind::DOUBLE));
		columns.push_back(Column("l_extendedprice",TypeKind::DOUBLE));
		columns.push_back(Column("l_discount",TypeKind::DOUBLE));
		columns.push_back(Column("l_tax",TypeKind::DOUBLE));
		columns.push_back(Column("l_returnflag",TypeKind::STRING));
		columns.push_back(Column("l_linestatus",TypeKind::STRING));
		columns.push_back(Column("l_shipdate",TypeKind::DATE));
		columns.push_back(Column("l_commitdate",TypeKind::DATE));
		columns.push_back(Column("l_receiptdate",TypeKind::DATE));
		columns.push_back(Column("l_shipinstruct",TypeKind::STRING));
		columns.push_back(Column("l_shipmode",TypeKind::STRING));
		columns.push_back(Column("l_comment",TypeKind::VARCHAR));
	}else if(tablename=="rle"){
		columns.push_back(Column("l_comment",TypeKind::INT));
		columns.push_back(Column("l_commitdate",TypeKind::INT));
		columns.push_back(Column("l_linenumber",TypeKind::INT));
		columns.push_back(Column("l_linestatus",TypeKind::INT));
		columns.push_back(Column("l_orderkey",TypeKind::INT));
		columns.push_back(Column("l_partkey",TypeKind::INT));
		columns.push_back(Column("l_receiptdate",TypeKind::INT));
		columns.push_back(Column("l_returnflag",TypeKind::INT));
		columns.push_back(Column("l_shipdate",TypeKind::INT));
		columns.push_back(Column("l_shipinstruct",TypeKind::INT));
		columns.push_back(Column("l_shipmode",TypeKind::INT));
		columns.push_back(Column("l_suppkey",TypeKind::INT));

	}else if(tablename=="rle2"||tablename=="testdelta"||tablename=="testrepeat"||tablename=="testpatched"||tablename=="testdirect"||tablename=="testint"||tablename=="testdelta2"){
		columns.push_back(Column("col",TypeKind::INT));
	}else if(tablename=="testdouble"){
		columns.push_back(Column("col",TypeKind::DOUBLE));
	}else{
		columns.push_back(Column("col",TypeKind::INT));
//		fprintf(stderr,"has no table defined! %s\n",table.c_str());
//		exit(0);
	}

	return new Table(tablename,tablepath,columns);
};

}

#endif /* INCLUDE_TABLE_H_ */
