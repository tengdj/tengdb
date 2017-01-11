// Copyright 2012 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <fstream>
#include <iostream>
#include <sstream>

#include <set>
#include <vector>

#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>

#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#include <llvm/IR/DataLayout.h>
#include <llvm/Linker/Linker.h>
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IRReader/IRReader.h"
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "config.h"
#include "util.h"
#include "llvm-codegen.h"

#include "MemoryPool.h"

using namespace llvm;
using std::fstream;


namespace tengdb {

static bool llvm_initialized = false;

void LlvmCodeGen::InitializeLlvm() {
  if (llvm_initialized) return;
  llvm::InitializeNativeTarget();
  InitializeNativeTargetAsmParser();
  InitializeNativeTargetAsmPrinter();
  llvm_initialized = true;
}

LlvmCodeGen::LlvmCodeGen(ObjectPool* pool, const string& id) :
  id_(id),
  optimizations_enabled_(true),
  optimizations_level_(3),
  is_corrupt_(false),
  is_compiled_(false),
  context_(new llvm::LLVMContext()),
  module_(0){


	void_type_ = llvm::Type::getVoidTy(context());
	ptr_type_ = PointerType::get(GetType(TypeKind::INT), 0);
	true_value_ = ConstantInt::get(context(), APInt(1, true, true));
	false_value_ = ConstantInt::get(context(), APInt(1, false, true));
	if(!module_){
		this->Init();
	}
	//loaded_functions_.resize(IRFunction::FN_END);
}

int LlvmCodeGen::LoadFromFile(ObjectPool* pool,const string& file, const string& id, LlvmCodeGen** codegen) {
  *codegen = new LlvmCodeGen(pool, id);
  (*codegen)->module_ = LoadModuleFromFile(*codegen, file).release();
  return (*codegen)->Init();
}

unique_ptr<Module> LlvmCodeGen::LoadModuleFromFile(LlvmCodeGen* codegen, const string& file) {
  llvm::SMDiagnostic error;
  unique_ptr<Module> module = llvm::parseIRFile(file,error,codegen->context());
  if(error.getLineNo()>0){
	  cout<<error.getMessage().str()<<endl;
	  cout<<error.getLineNo()<<"\t"<<error.getLineContents().str()<<endl;
  }
  return std::move(module);
}

int LlvmCodeGen::LinkModule(const string& file) {
//	//this file already been linked
//  if (linked_modules_.find(file) != linked_modules_.end()) return 1;
//
//  unique_ptr<Module> new_module = LoadModuleFromFile(this, file);
//  string error_msg;
//
//  bool error =
//      Linker::linkModules(*module_, new_module);
//  if (error) {
//    stringstream ss;
//    ss << "Problem linking " << file << " to main module: " << error_msg;
//    return 0;
//  }
//  linked_modules_.insert(file);
  return 1;
}


void initDecodeBitWidth(LlvmCodeGen *gen){
	ArrayType *arrayType = ArrayType::get(gen->int_type(32),32);
	Module *mod = gen->module();
	std::vector<Constant *> elements;
	for(int i=0;i<24;i++){
		elements.push_back(gen->getConstant(32,i+1));//0
	}
	elements.push_back(gen->getConstant(32,26));//24
	elements.push_back(gen->getConstant(32,28));//25
	elements.push_back(gen->getConstant(32,30));//26
	elements.push_back(gen->getConstant(32,32));//27
	elements.push_back(gen->getConstant(32,40));//28
	elements.push_back(gen->getConstant(32,48));//29
	elements.push_back(gen->getConstant(32,56));//30
	elements.push_back(gen->getConstant(32,64));//31

	Constant *constant = ConstantArray::get(arrayType,elements);
	GlobalVariable *gvar_decodeBitWidth = new GlobalVariable(*mod,arrayType,false,GlobalValue::ExternalLinkage,constant,"decodeBitWidth");
	gvar_decodeBitWidth->setAlignment(16);


}

void initClosestBits(LlvmCodeGen *gen){

	ArrayType *arrayType = ArrayType::get(gen->int_type(32),64);
	Module *mod = gen->module();
	std::vector<Constant *> elements;

	elements.push_back(gen->getConstant(32,1));//0
	for(int i=1;i<25;i++){
		elements.push_back(gen->getConstant(32,i));//1
	}
	elements.push_back(gen->getConstant(32,26));//25
	elements.push_back(gen->getConstant(32,26));//26
	elements.push_back(gen->getConstant(32,28));//27
	elements.push_back(gen->getConstant(32,28));//28
	elements.push_back(gen->getConstant(32,30));//29
	elements.push_back(gen->getConstant(32,30));//30
	elements.push_back(gen->getConstant(32,32));//31
	elements.push_back(gen->getConstant(32,32));//32
	elements.push_back(gen->getConstant(32,40));//33
	elements.push_back(gen->getConstant(32,40));//34
	elements.push_back(gen->getConstant(32,40));//35
	elements.push_back(gen->getConstant(32,40));//36
	elements.push_back(gen->getConstant(32,40));//37
	elements.push_back(gen->getConstant(32,40));//38
	elements.push_back(gen->getConstant(32,40));//39
	elements.push_back(gen->getConstant(32,40));//40
	elements.push_back(gen->getConstant(32,48));//41
	elements.push_back(gen->getConstant(32,48));//42
	elements.push_back(gen->getConstant(32,48));//43
	elements.push_back(gen->getConstant(32,48));//44
	elements.push_back(gen->getConstant(32,48));//45
	elements.push_back(gen->getConstant(32,48));//46
	elements.push_back(gen->getConstant(32,48));//47
	elements.push_back(gen->getConstant(32,48));//48
	elements.push_back(gen->getConstant(32,56));//49
	elements.push_back(gen->getConstant(32,56));//50
	elements.push_back(gen->getConstant(32,56));//51
	elements.push_back(gen->getConstant(32,56));//52
	elements.push_back(gen->getConstant(32,56));//53
	elements.push_back(gen->getConstant(32,56));//54
	elements.push_back(gen->getConstant(32,56));//55
	elements.push_back(gen->getConstant(32,56));//56
	elements.push_back(gen->getConstant(32,64));//57
	elements.push_back(gen->getConstant(32,64));//58
	elements.push_back(gen->getConstant(32,64));//59
	elements.push_back(gen->getConstant(32,64));//60
	elements.push_back(gen->getConstant(32,64));//61
	elements.push_back(gen->getConstant(32,64));//62
	elements.push_back(gen->getConstant(32,64));//63

	Constant *constant = ConstantArray::get(arrayType,elements);
	GlobalVariable *gvar_fixedBits = new GlobalVariable(*mod,arrayType,false,GlobalValue::ExternalLinkage,constant,"closestFixedBits");
	gvar_fixedBits->setAlignment(16);

}

int LlvmCodeGen::Init() {
  InitializeLlvm();

  if (module_ == NULL) {
    module_ = new Module(id_, context());
    module_->setDataLayout("");
    module_->setTargetTriple("x86_64-unknown-linux-gnu");
    initClosestBits(this);
    initDecodeBitWidth(this);
  }




  llvm::CodeGenOpt::Level opt_level = (CodeGenOpt::Level)FLAGS_optimization_level;
#ifndef NDEBUG
  // For debug builds, don't generate JIT compiled optimized assembly.
  // This takes a non-neglible amount of time (~.5 ms per function) and
  // blows up the fe tests (which take ~10-20 ms each).
  opt_level = CodeGenOpt::None;
#endif

  std::string errmessage;
  llvm::ExecutionEngine *ee =
  			llvm::EngineBuilder(unique_ptr<Module>(module_)).setMCPU(llvm::sys::getHostCPUName())
  			.setOptLevel(opt_level)
  			.setErrorStr(&error_string_)
  			.setEngineKind(EngineKind::JIT)
  			.setMCJITMemoryManager(std::unique_ptr<RTDyldMemoryManager>(new SectionMemoryManager()))
  			.create();

  //TODO Uncomment the below line as soon as we upgrade to LLVM 3.5 to enable SSE, if
  // available. In LLVM 3.3 this is done automatically and cannot be enabled because
  // for some reason SSE4 intrinsics selection will not work.
  //builder.setMCPU(llvm::sys::getHostCPUName());

  	if(!ee){
  		cerr<<"llvm-codegen.cc:Init "<<errmessage<<endl;
  	}
    execution_engine_.reset(ee);
    if (execution_engine_ == NULL) {
    // execution_engine_ will take ownership of the module if it is created
    //delete module_;
    cerr << "Could not create ExecutionEngine: " << error_string_;
    return 0;
  }


  return 1;
}

LlvmCodeGen::~LlvmCodeGen() {
	execution_engine_->removeModule(this->module_);
	/*
  for (map<Function*, bool>::iterator iter = jitted_functions_.begin();
      iter != jitted_functions_.end(); ++iter) {
    execution_engine_->freeMachineCodeForFunction(iter->first);
    execution_engine_->
  }
  */
}

void LlvmCodeGen::EnableOptimizations(bool enable) {
  optimizations_enabled_ = enable;
}

string LlvmCodeGen::GetIR(bool full_module) const {
  string str;
  raw_string_ostream stream(str);
  if (full_module) {
    module_->print(stream, NULL);
  } else {
    for (int i = 0; i < codegend_functions_.size(); ++i) {
      codegend_functions_[i]->print(stream, NULL);
    }
  }
  return str;
}

llvm::Type* LlvmCodeGen::GetType(const TypeKind & type) {
  switch (type) {

    case TypeKind::BINARY:
      return llvm::Type::getInt1Ty(context());
    case TypeKind::BYTE:
      return llvm::Type::getInt8Ty(context());
    case TypeKind::SHORT:
      return llvm::Type::getInt16Ty(context());
    case TypeKind::INT:
      return llvm::Type::getInt32Ty(context());
    case TypeKind::LONG:
      return llvm::Type::getInt64Ty(context());
    case TypeKind::FLOAT:
      return llvm::Type::getFloatTy(context());
    case TypeKind::DOUBLE:
      return llvm::Type::getDoubleTy(context());
    case TypeKind::STRING:
    case TypeKind::CHAR:
    case TypeKind::VARCHAR:
      return string_val_type_;
    case TypeKind::TIMESTAMP:
    case TypeKind::DATE:
      return timestamp_val_type_;
    default:
      return NULL;
  }
}

llvm::Type* LlvmCodeGen::GetType(const string& name) {
  llvm::Type* type = module_->getTypeByName(name);
  return type;
}

PointerType* LlvmCodeGen::GetPtrType(const string& name) {
  llvm::Type* type = GetType(name);
  return PointerType::get(type, 0);
}

PointerType* LlvmCodeGen::GetPtrType(llvm::Type* type) {
  return PointerType::get(type, 0);
}

// Llvm doesn't let you create a PointerValue from a c-side ptr.  Instead
// cast it to an int and then to 'type'.
Value* LlvmCodeGen::CastPtrToLlvmPtr(llvm::Type* type, const void* ptr) {
  Constant* const_int = ConstantInt::get(llvm::Type::getInt64Ty(context()), (int64_t)ptr);
  return ConstantExpr::getIntToPtr(const_int, type);
}

Value* LlvmCodeGen::GetIntConstant(TypeKind type, int64_t val) {
  switch (type) {
    case TypeKind::BYTE:
      return ConstantInt::get(context(), APInt(8, val));
    case TypeKind::SHORT:
      return ConstantInt::get(context(), APInt(16, val));
    case TypeKind::INT:
      return ConstantInt::get(context(), APInt(32, val));
    case TypeKind::LONG:
      return ConstantInt::get(context(), APInt(64, val));
    default:
      return NULL;
  }
}

AllocaInst* LlvmCodeGen::CreateEntryBlockAlloca(Function* f, const NamedVariable& var) {
  IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
  AllocaInst* alloca = tmp.CreateAlloca(var.type, 0, var.name.c_str());
  //TODO ignore decimal
  /*
  if (var.type == llvm::Type::TypeID::ArrayTyID) {
    // Generated functions may manipulate DecimalVal arguments via SIMD instructions such
    // as 'movaps' that require 16-byte memory alignment. LLVM uses 8-byte alignment by
    // default, so explicitly set the alignment for DecimalVals.
    alloca->setAlignment(16);
  }*/
  return alloca;
}

AllocaInst* LlvmCodeGen::CreateEntryBlockAlloca(const LlvmBuilder& builder, llvm::Type* type,
                                                const char* name) {
  return CreateEntryBlockAlloca(builder.GetInsertBlock()->getParent(),
                                NamedVariable(name, type));
}

void LlvmCodeGen::CreateIfElseBlocks(Function* fn, const string& if_name,
    const string& else_name, BasicBlock** if_block, BasicBlock** else_block,
    BasicBlock* insert_before) {
  *if_block = BasicBlock::Create(context(), if_name, fn, insert_before);
  *else_block = BasicBlock::Create(context(), else_name, fn, insert_before);
}

Function* LlvmCodeGen::GetLibCFunction(FnPrototype* prototype) {
  if (external_functions_.find(prototype->name()) != external_functions_.end()) {
    return external_functions_[prototype->name()];
  }
  Function* func = prototype->GeneratePrototype();
  external_functions_[prototype->name()] = func;
  return func;
}

uint64_t LlvmCodeGen::GetFunction(string functionname) {
	return this->execution_engine_->getFunctionAddress(functionname);
}

LlvmCodeGen::FnPrototype::FnPrototype(
    LlvmCodeGen* gen, const string& name, llvm::Type* ret_type, bool isinline) :
  codegen_(gen), name_(name), ret_type_(ret_type) ,inline_(isinline){
	if(codegen_->is_compiled_){
		cout<<"Not valid to add additional functions\n";
	}
   assert(!codegen_->is_compiled_);
}

Function* LlvmCodeGen::FnPrototype::GeneratePrototype(
      LlvmBuilder* builder, Value** params) {
  vector<llvm::Type*> arguments;
  for (int i = 0; i < args_.size(); ++i) {
    arguments.push_back(args_[i].type);
  }
  FunctionType* prototype = FunctionType::get(ret_type_, arguments, false);

  Function* fn;


  AttrBuilder B;
  B.addAttribute(Attribute::NoUnwind);
  B.addAttribute(Attribute::UWTable);

  if(inline_){
	  fn = Function::Create(
	  	       prototype, Function::LinkOnceODRLinkage, name_, codegen_->module_);
	  B.addAttribute(Attribute::InlineHint);
  }else{
	  fn = Function::Create(
	       prototype, Function::ExternalLinkage, name_, codegen_->module_);
  }
  AttributeSet attrs = AttributeSet::get(codegen_->context(), ~0U, B);
  fn->setAttributes(attrs);

  assert(fn != NULL);

  // Name the arguments
  int idx = 0;
  for (Function::arg_iterator iter = fn->arg_begin();
      iter != fn->arg_end(); ++iter, ++idx) {
    iter->setName(args_[idx].name);
    if (params != NULL) params[idx] = (Argument *)iter;
  }

  if (builder != NULL) {
    BasicBlock* entry_block = BasicBlock::Create(codegen_->context(), "entry", fn);
    builder->SetInsertPoint(entry_block);
  }

  codegen_->codegend_functions_.push_back(fn);
  return fn;
}

Function* LlvmCodeGen::ReplaceCallSites(Function* caller, bool update_in_place,
    Function* new_fn, const string& replacee_name, int* replaced) {
  assert(caller->getParent() == module_);
  assert(caller != NULL);
  assert(new_fn != NULL);

  if (!update_in_place) {
    caller = CloneFunction(caller);
  } else if (jitted_functions_.find(caller) != jitted_functions_.end()) {
    // This function is already dynamically linked, unlink it.
    //execution_engine_->freeMachineCodeForFunction(caller);

    jitted_functions_.erase(caller);
  }

  *replaced = 0;

  for (inst_iterator iter = inst_begin(caller); iter != inst_end(caller); ++iter) {
      Instruction* instr = &*iter;
      // look for call instructions
      if (CallInst::classof(instr)) {
        CallInst* call_instr = reinterpret_cast<CallInst*>(instr);
        Function* old_fn = call_instr->getCalledFunction();
        // look for call instruction that matches the name
        if (old_fn != NULL && old_fn->getName().find(replacee_name) != string::npos) {
          // Replace the called function
          call_instr->setCalledFunction(new_fn);
          ++*replaced;
        }
      }
    }

  if(!update_in_place){
	  this->AddFunctionToJit(caller,(void**)caller);
  }
  return caller;
}

Function* LlvmCodeGen::CloneFunction(Function* fn) {
  ValueToValueMapTy dummy_vmap;
  // CloneFunction() automatically gives the new function a unique name
  Function* fn_clone = llvm::CloneFunction(fn, dummy_vmap);
  fn_clone->copyAttributesFrom(fn);
  //module_->getFunctionList().push_back(fn_clone);
  return fn_clone;
}

// TODO: revisit this. Inlining all call sites might not be the right call.  We
// probably need to make this more complicated and somewhat cost based or write
// our own optimization passes.
int LlvmCodeGen::InlineCallSites(Function* fn, bool skip_registered_fns) {
  int functions_inlined = 0;
  // Collect all call sites
  vector<CallInst*> call_sites;

  // loop over all instructions
  for (inst_iterator iter = inst_begin(fn); iter != inst_end(fn); ++iter) {
      Instruction* instr = &*iter;;
      // look for call instructions
      if (CallInst::classof(instr)) {
        CallInst* call_instr = reinterpret_cast<CallInst*>(instr);
        Function* called_fn = call_instr->getCalledFunction();
        // called_fn will be NULL if it's a virtual function call, etc.

        if (called_fn == NULL || !called_fn->hasFnAttribute(Attribute::AlwaysInline)) {
          continue;
        }
        if (skip_registered_fns) {
          if (registered_exprs_.find(called_fn) != registered_exprs_.end()) {
            continue;
          }
        }
        call_sites.push_back(call_instr);
      }
  }

  // Inline all call sites.  InlineFunction can still fail (function is recursive, etc)
  // but that always leaves the original function in a consistent state
  for (int i = 0; i < call_sites.size(); ++i) {
    llvm::InlineFunctionInfo info;
    if (llvm::InlineFunction(call_sites[i], info)) {
      ++functions_inlined;
    }
  }
  return functions_inlined;
}

Function* LlvmCodeGen::OptimizeFunctionWithExprs(Function* fn) {
  int num_inlined;
  do {
    // This assumes that all redundant exprs have been registered.
    num_inlined = InlineCallSites(fn, false);
  } while (num_inlined > 0);

  // TODO(skye): fix subexpression elimination
  // SubExprElimination subexpr_elim(this);
  // subexpr_elim.Run(fn);
  return FinalizeFunction(fn);
}

Function* LlvmCodeGen::FinalizeFunction(Function* function) {
  function->addFnAttr(llvm::Attribute::AlwaysInline);
/*
  if (!VerifyFunction(function)) {
    function->eraseFromParent(); // deletes function
    return NULL;
  }*/
  //if (FLAGS_dump_ir) function->dump();
  return function;
}

int LlvmCodeGen::FinalizeModule() {
  assert(!is_compiled_);
  is_compiled_ = true;
  if (irpath.size() != 0) {
    string path = irpath + "/" + id_ + "_unopt.ll";
    fstream f(path.c_str(), fstream::out | fstream::trunc);
    if (f.fail()) {
      cerr << "Could not save IR to: " << path <<endl;
    } else {
      f << GetIR(true);
      f.close();
    }
  }

  if (is_corrupt_) return 0;

  //module_->dump();

  // Don't waste time optimizing module if there are no functions to JIT. This can happen
  // if the codegen object is created but no functions are successfully codegen'd.
  if (FLAGS_optimization_level>0&&optimizations_enabled_ && !fns_to_jit_compile_.empty()) {
    OptimizeModule();
  }

  //module_->dump();

  // JIT compile all codegen'd functions
  for (int i = 0; i < fns_to_jit_compile_.size(); ++i) {
    *fns_to_jit_compile_[i].second = JitFunction(fns_to_jit_compile_[i].first);
  }

  if (irpath.size() != 0) {
    string path = irpath + "/" + id_ + "_opt.ll";
    fstream f(path.c_str(), fstream::out | fstream::trunc);
    if (f.fail()) {
      cerr << "Could not save IR to: " << path<<endl;;
    } else {
      f << GetIR(true);
      f.close();
    }
  }

  return 1;
}

bool globalvalue(const llvm::GlobalValue &value){
	return true;
}




void LlvmCodeGen::OptimizeModule() {

  // This pass manager will construct optimizations passes that are "typical" for
  // c/c++ programs.  We're relying on llvm to pick the best passes for us.
  // TODO: we can likely muck with this to get better compile speeds or write
  // our own passes.  Our subexpression elimination optimization can be rolled into
  // a pass.
  PassManagerBuilder pass_builder ;
  // 2 maps to -O2
  // TODO: should we switch to 3? (3 may not produce different IR than 2 while taking
  // longer, but we should check)
  pass_builder.OptLevel = optimizations_level_;
  // Don't optimize for code size (this corresponds to -O2/-O3)
  pass_builder.SizeLevel = 0;
  pass_builder.Inliner = createFunctionInliningPass() ;


  TargetIRAnalysis target_analysis =
      execution_engine_->getTargetMachine()->getTargetIRAnalysis();
  // Before running any other optimization passes, run the internalize pass, giving it
  // the names of all functions registered by AddFunctionToJit(), followed by the
  // global dead code elimination pass. This causes all functions not registered to be
  // JIT'd to be marked as internal, and any internal functions that are not used are
  // deleted by DCE pass. This greatly decreases compile time by removing unused code.
//  vector<const char*> exported_fn_names;
//  for (int i = 0; i < fns_to_jit_compile_.size(); ++i) {
//    exported_fn_names.push_back(fns_to_jit_compile_[i].first->getName().data());
//  }
  unique_ptr<legacy::PassManager> module_pass_manager(new legacy::PassManager());
  module_pass_manager->add(createInternalizePass(globalvalue));
  module_pass_manager->add(createGlobalDCEPass());
  module_pass_manager->add(createTargetTransformInfoWrapperPass(target_analysis));

  module_pass_manager->run(*module_);

  // Create and run function pass manager
  unique_ptr<legacy::FunctionPassManager> fn_pass_manager(new legacy::FunctionPassManager(module_));
  pass_builder.populateFunctionPassManager(*fn_pass_manager);
  fn_pass_manager->doInitialization();
  fn_pass_manager->add(llvm::createInstructionCombiningPass());
  fn_pass_manager->add(llvm::createReassociatePass());
  fn_pass_manager->add(llvm::createGVNPass());
  fn_pass_manager->add(llvm::createCFGSimplificationPass());
  fn_pass_manager->add(llvm::createAggressiveDCEPass());
  for (Module::iterator it = module_->begin(), end = module_->end(); it != end ; ++it) {
    if (!it->isDeclaration()) {
    	fn_pass_manager->run(*it);
    }
  }
  fn_pass_manager->doFinalization();

  // Create and run module pass manager
  module_pass_manager.reset(new legacy::PassManager());

  pass_builder.populateModulePassManager(*module_pass_manager);
  module_pass_manager->run(*module_);
}

void LlvmCodeGen::AddFunctionToJit(Function* fn, void** fn_ptr) {

//  llvm::Type* decimal_val_type = GetType(TypeKind::DECIMAL);
//  if (fn->getReturnType() == decimal_val_type) {
//    // Per the x86 calling convention ABI, DecimalVals should be returned via an extra
//    // first DecimalVal* argument. We generate non-compliant functions that return the
//    // DecimalVal directly, which we can call from generated code, but not from compiled
//    // native code.  To avoid accidentally calling a non-compliant function from native
//    // code, call 'function' from an ABI-compliant wrapper.
//    stringstream name;
//    name << fn->getName().str() << "ABIWrapper";
//    LlvmCodeGen::FnPrototype prototype(this, name.str(), void_type_);
//    // Add return argument
//    prototype.AddArgument(NamedVariable("result", decimal_val_type->getPointerTo()));
//    // Add regular arguments
//    for (Function::arg_iterator arg = fn->arg_begin(); arg != fn->arg_end(); ++arg) {
//      prototype.AddArgument(NamedVariable(arg->getName(), arg->getType()));
//    }
//    LlvmBuilder builder(context());
//    Value* args[fn->arg_size() + 1];
//    Function* fn_wrapper = prototype.GeneratePrototype(&builder, &args[0]);
//    fn_wrapper->addFnAttr(llvm::Attribute::AlwaysInline);
//    // Mark first argument as sret (not sure if this is necessary but it can't hurt)
//    fn_wrapper->addAttribute(1, Attribute::StructRet);
//    // Call 'fn' and store the result in the result argument
//    Value* result =
//        builder.CreateCall(fn, ArrayRef<Value*>(&args[1], fn->arg_size()), "result");
//    builder.CreateStore(result, args[0]);
//    builder.CreateRetVoid();
//    fn = FinalizeFunction(fn_wrapper);
//    assert(fn != NULL);
//  }
  fns_to_jit_compile_.push_back(make_pair(fn, fn_ptr));
}

void* LlvmCodeGen::JitFunction(Function* function) {
  if (is_corrupt_) return NULL;

  // TODO: log a warning if the jitted function is too big (larger than I cache)
  void* jitted_function = execution_engine_->getPointerToFunction(function);
  if (jitted_function != NULL) {
    jitted_functions_[function] = true;
  }
  return jitted_function;
}


void LlvmCodeGen::GetFunctions(vector<Function*>* functions) {
  Module::iterator fn_iter = module_->begin();
  while (fn_iter != module_->end()) {
    Function* fn = (Function *)fn_iter++;
    if (!fn->empty()) functions->push_back(fn);
  }
}

void LlvmCodeGen::GetSymbols(vector<string>* symbols) {
  Module::iterator fn_iter = module_->begin();
  while (fn_iter != module_->end()) {
    Function* fn = (Function *)fn_iter++;
    if (!fn->empty()) symbols->push_back(fn->getName().str());
  }
}

/*
Value* LlvmCodeGen::CodegenArrayAt(LlvmBuilder* builder, Value* array, int idx,
    const char* name) {
  assert(array->getType()->isPointerTy() || array->getType()->isArrayTy())
      << Print(array->getType());
  Value* ptr = builder->CreateConstGEP1_32(array, idx);
  return builder->CreateLoad(ptr, name);
}
*/

void LlvmCodeGen::ReplaceInstWithValue(Instruction* from, Value* to) {
  BasicBlock::iterator iter(from);
  llvm::ReplaceInstWithValue(from->getParent()->getInstList(), iter, to);
}

Argument* LlvmCodeGen::GetArgument(Function* fn, int i) {
  assert(i<=fn->arg_size());
  Function::arg_iterator iter = fn->arg_begin();
  //inst_iterator iter = inst_begin(fn);
  for (int j = 0; j < i; ++j) ++iter;
  return (Argument*)iter;
}

Value* LlvmCodeGen::GetPtrTo(LlvmBuilder* builder, Value* v, const char* name) {
  Value* ptr = CreateEntryBlockAlloca(*builder, v->getType(), name);
  builder->CreateStore(v, ptr);
  return ptr;
}

ConstantInt *LlvmCodeGen::getConstant(uint64_t bitsize, uint64_t value){
	return ConstantInt::get(IntegerType::get(context(),bitsize),value);
}


}
