import sys
import os
import string
import random
import collections
import cPickle
import re


class TextGenerator(object):
    min_text_length = 10000
    generated_text = []
    corpus_path = './corpus/'

    def __init__(self):
        data_list = []
        self.get_training_data()
        texts = cPickle.load(open("data.p", "rb"))
        for text in texts:
            clean_data = re.sub(', ',
                                ' _COMMA_ ',
                                re.sub('\?',
                                       '?.',
                                       re.sub('\xe2\x80\x99',
                                              '_QUOTE_',
                                              text.lower())))
            clean_data = re.sub("([\s\'][\s\'\.]+)",
                                ' ',
                                re.sub('_QUOTE_',
                                       "'",
                                       re.sub('[^\x00-\x7F]+',
                                              ' ',
                                              clean_data)))
            clean_data = re.sub('[\n\*]+',
                                '.',
                                re.sub(" i'",
                                       " I'",
                                       re.sub(' i ',
                                              ' I ',
                                              re.sub('[\ \:\/\;\(\)\"\[\]]+',
                                                     ' ',
                                                     re.sub('[\ -][-\ ]',
                                                            ' ',
                                                            clean_data)))))
            data_list.append(re.split('[\.\!]', clean_data))
        data_list = list(flatten_list(data_list))

        chain = {}
        for sentence in data_list:
            sentence = re.sub('[\s]+',
                              ' ',
                              re.sub('^',
                                     'BEGIN NOW ',
                                     re.sub('$',
                                            ' END',
                                            sentence)))
            sequences = self.generate_word_sequences(sentence.split())
            for word1, word2, word3 in sequences:
                key = (word1, word2)
                if key in chain:
                    chain[key].append(word3)
                else:
                    chain[key] = [word3]

        training_chain = {}
        for keys in chain:
            if len(chain[keys]) > 2:
                training_chain[keys] = [chain[keys]]

        cPickle.dump(chain, open("sequences.p", "wb"))

    def get_training_data(self):
        texts = []
        for root, dirs, files in os.walk(self.corpus_path):
            for name in files:
                with open(root + '/' + name, 'rb') as input_file:
                    text = input_file.read()
                    texts.append(text)
        cPickle.dump(texts, open("data.p", "wb"))

    def generate_word_sequences(self, sentence):
        if len(sentence) < 6:
            return
        for i in xrange(len(sentence) - 2):
            yield (sentence[i], sentence[i + 1], sentence[i + 2])

    def generate_text(self):
        chain = cPickle.load(open("sequences.p", "rb"))

        text_length = 0
        paragraphs_count = 1

        while(text_length < self.min_text_length):
            sentence = []
            sword1 = "BEGIN"
            sword2 = "NOW"
            while True:
                sword1, sword2 = sword2, random.choice(chain[(sword1, sword2)])
                if sword2 == "END":
                    break
                sentence.append(sword2)
            sentence[0] = sentence[0].capitalize()
            text_sentence = re.sub(' _COMMA_',
                                   ',',
                                   re.sub('\?\.',
                                          '?',
                                          (' '.join(sentence)) + ". "))
            self.generated_text.append(text_sentence)
            text_length += len(sentence)
            if text_length > paragraphs_count * random.randint(10, 50) * 5:
                paragraphs_count += 1
                self.generated_text.append('\n')

    def output_generated_text(self):
        print ''.join(self.generated_text)


def flatten_list(item):
    for i in (item):
        if (isinstance(i, collections.Iterable) and not isinstance(i, str)):
            for j in flatten_list(i):
                yield j
        else:
            yield i


def main():
    text_generator = TextGenerator()
    text_generator.generate_text()
    text_generator.output_generated_text()


if __name__ == '__main__':
    main()
