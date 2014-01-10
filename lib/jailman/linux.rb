#encoding: utf-8
module Jailman
  class Linux
    class << self
      def hi
        create_namespace_for("ls -al", ".")
      end
    end
  end
end
