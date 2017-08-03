#include <boost/lexical_cast.hpp>
#include <iostream>
#include "caf/all.hpp"

using SubscribeRtnOrderAtom = caf::atom_constant<caf::atom("subro")>;

caf::behavior foo(caf::event_based_actor* self) {
  self->set_down_handler(
      [](const caf::down_msg& msg) { std::cout << "down msg:\n"; });

  return {[=](const caf::strong_actor_ptr& actor) { self->monitor(actor); }};
}

class Bar : public caf::event_based_actor {
 public:
  Bar(caf::actor_config& cfg) : caf::event_based_actor(cfg) {
    // grp_ = system().groups().anonymous();

    std::string module = "local";
    std::string id = "foo";
    auto expected_grp = system().groups().get(module, id);
    if (!expected_grp) {
      std::cerr << "*** cannot load group: "
                << system().render(expected_grp.error()) << std::endl;
      return;
    }
    auto grp = std::move(*expected_grp);
    join(grp);
  }

  caf::behavior make_behavior() override {
    set_exit_handler([=](caf::scheduled_actor* self, const caf::exit_msg& em) {
      grp_ = nullptr;
      caf::aout(this) << "errr\n";
    });
    return {[=](bool) { std::cout << "abc\n"; },
            [=](const std::string& str) {
              send(grp_, str);
              quit();
            },
            [=](SubscribeRtnOrderAtom) { return grp_; },
            [=](const caf::exit_msg& msg) { caf::aout(this) << "errr\n"; }};
  }

 private:
  caf::group grp_;
};

class Foo : public caf::event_based_actor {
 public:
  Foo(caf::actor_config& cfg, const caf::actor& buddy)
      : caf::event_based_actor(cfg), buddy_(buddy) {
    // grp_ = system().groups().anonymous();

    std::string module = "local";
    std::string id = "foo";
    auto expected_grp = system().groups().get(module, id);
    if (!expected_grp) {
      std::cerr << "*** cannot load group: "
                << system().render(expected_grp.error()) << std::endl;
      return;
    }
    auto grp = std::move(*expected_grp);
    join(grp);
    leave(grp);
  }

  caf::behavior make_behavior() override {
    set_exit_handler([=](caf::scheduled_actor* self, const caf::exit_msg& em) {
      grp_ = nullptr;
      caf::aout(this) << "errr\n";
      //       quit();
    });
    send(buddy_, caf::actor_cast<caf::strong_actor_ptr>(this));
    return {[=](bool) { std::cout << "abc\n"; },
            [=](const std::string& str) {
              send(grp_, str);
              quit();
            },
            [=](SubscribeRtnOrderAtom) { return grp_; },
            [=](const caf::exit_msg& msg) { caf::aout(this) << "errr\n"; }};
  }

 private:
  caf::group grp_;
  caf::actor buddy_;
};

int caf_main(caf::actor_system& system, const caf::actor_system_config& cfg) {
  // auto actor = system.spawn<foo>();
  auto actor = system.spawn(foo);
  auto f = system.spawn<Foo>(actor);
  // caf::anon_send_exit(actor, caf::exit_reason::user_shutdown);

  system.await_all_actors_done();
  return 0;
}

CAF_MAIN()
