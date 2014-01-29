#encoding: utf-8
module Jailman
  class CommandRunner

    class << self

      include Jailman::Constants

      def run(command)
        output = `#{command}`
        raise output unless $? == 0
        output
      end

      def create_jail(jail)
        pid = run("#{JAIL_SCRIPT} #{jail.name} --create")
        run("echo #{pid.chomp} > #{PID_DIR}/#{jail.name}.pid")
        pid
      end

      def create_rootfs(jail)
        run("#{ROOTFS_SCRIPT} #{jail.name} #{jail.directory}")
      end

      def setup_network(jail)
        run("#{NETWORK_SCRIPT} #{jail.name}")
      end

      def kill_process(jail)
        run("#{JAIL_SCRIPT} #{jail.name} --kill")
      end

      def remove_pid_file(jail)
        run("rm -f #{PID_DIR}/#{jail.name}.pid")
      end

      def remove_rootfs(jail)
        run("rm -rf #{jail.directory}")
      end

    end

  end
end
