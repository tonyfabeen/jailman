require 'spec_helper'
require 'jailman/linux'

describe Jailman::Linux do
  describe 'create_namespace' do
    it { expect(Jailman::Linux.create_namespace_for("A","B")).to be_true}
  end
end
