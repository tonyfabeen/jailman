#encoding: utf-8

module Jailman
  class Provisioner

    attr_reader :jail

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
    end

    def run!
      jail.pid = Jailman::CommandRunner.create_jail(jail)
      Jailman::CommandRunner.create_rootfs(jail)
      Jailman::CommandRunner.setup_network(jail)
      save_state
    end

    def destroy!
      Jailman::CommandRunner.kill_process(jail)
      Jailman::CommandRunner.remove_pid_file(jail)
      sleep 1
      Jailman::CommandRunner.remove_rootfs(jail)
      clear_state
    end

    private

    def save_state
      state = Jailman::State.new(jail)
      state.save
    end

    def clear_state
      state = Jailman::State.new(jail)
      state.clear
    end

  end
end
