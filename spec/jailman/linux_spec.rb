require 'spec_helper'
require 'jailman/linux'

describe Jailman::Linux do
  describe 'clone' do
    it { expect(described_class.hi).to be_true}
  end
end
