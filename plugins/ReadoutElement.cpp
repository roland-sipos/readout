/**
 * @file ReadoutElement.cpp ReadoutElement class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/readoutelement/Nljs.hpp"

#include "ReadoutElement.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <sstream>
#include <memory>
#include <string>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "ReadoutElement" // NOLINT

namespace dunedaq {
namespace readout { 

ReadoutElement::ReadoutElement(const std::string& name)
  : DAQModule(name)
  , configured_(false)
  , queue_timeout_ms_(2000)
  , worker_thread_(std::bind(&ReadoutElement::do_work, this, std::placeholders::_1))
  , input_queue_(nullptr)
  , readout_context_impl_(nullptr)
  , run_marker_{false}
  , packet_count_{0}
  , stats_thread_(0)
{
  register_command("conf", &ReadoutElement::do_conf);
  register_command("start", &ReadoutElement::do_start);
  register_command("stop", &ReadoutElement::do_stop);
}

void
ReadoutElement::init(const data_t& args)
{
  auto ini = args.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.dir != "input") {
      continue;
    }
    ERS_INFO("Resetting queue: " << qi.inst);
    try {
      input_queue_.reset(new source_t(qi.inst));
    }
    catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, get_name(), qi.name, excpt);
    }
  }
}

void
ReadoutElement::do_conf(const data_t& args)
{
  queue_timeout_ms_ = std::chrono::milliseconds(2000);
  std::string rawtype("wib");
  //readout_context_impl_ = ReadoutContextMaker(rawtype, run_marker_);
  readout_context_impl_ = std::make_unique<ReadoutContext<types::WIB_SUPERCHUNK_STRUCT>>(rawtype, run_marker_);
  readout_context_impl_->conf(args);
}

void 
ReadoutElement::do_start(const data_t& args)
{
  run_marker_.store(true);
  stats_thread_.set_work(&ReadoutElement::run_stats, this);
  readout_context_impl_->start(args);
  worker_thread_.start_working_thread();
}

void 
ReadoutElement::do_stop(const data_t& args)
{
  run_marker_.store(false);
  readout_context_impl_->stop(args);
  worker_thread_.stop_working_thread();
  while (!stats_thread_.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));          
  }
}

void
ReadoutElement::do_work(std::atomic<bool>& working_flag)
{
  while (working_flag.load()) {
    std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT> payload_ptr;
    try {
      input_queue_->pop(payload_ptr, queue_timeout_ms_);
    } 
    catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      std::runtime_error("Queue Source timed out...");
    }
    readout_context_impl_->handle(std::move(payload_ptr));
    ++packet_count_;
  }
}

void
ReadoutElement::run_stats()
{
  // Temporarily, for debugging, a rate checker thread...
  int new_packets = 0;
  auto t0 = std::chrono::high_resolution_clock::now();
  while (run_marker_.load()) {
    auto now = std::chrono::high_resolution_clock::now();
    new_packets = packet_count_.exchange(0);
    double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
    ERS_INFO("Packet rate: " << new_packets/seconds/1000. << " [kHz]");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    t0 = now;
  }
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::ReadoutElement)